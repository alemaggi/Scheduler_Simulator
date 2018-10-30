//
//  main.c
//  ProvaListFileScheduler
//
//  Created by Alessandro Maggi on 30/10/2018.
//  Copyright © 2018 Alessandro Maggi. All rights reserved.
//

/*
 file contenente tutte le strutture dati necessare alla gesitone dei task e delle istruzioni
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

typedef struct Task task; /*I task sono una lista concatenata di istruzioni*/
typedef struct Instruction instruction;
typedef struct List_for_instruction listInstruction; /*Lista per contenere le istruzioni*/
typedef struct Queue_for_task queueTask; /*coda per contenere i task*/

/*Struttura per rappresentare un task*/
struct Task {
    int id;
    instruction *programCounter;
    int arrival_time;
    listInstruction *instr_list; //lista delle istruzioni da eseguire instr_list
    int processState; /*usiamo un numero per rappresentare i 5 stati*/
    task *next;
};

/*Struttura per rappresentare una istruzione*/
struct Instruction {
    bool typeFlag;
    int length;
    instruction *next;
};

/*Struttura per rappresentare la lista di istruzioni che contiene solo un puntatore all' istruzione successiva*/
struct List_for_instruction {
    instruction *headInstruction;
};

/*Struttura per rappresentare la lista di istruzioni che contiene solo un puntatore all' istruzione successiva*/
struct Queue_for_task {
    task *firstTask;
};

/*nuova lista di istruzioni*/
listInstruction *newListInstructionNode() {
    listInstruction *newListNode = NULL;
    newListNode = (listInstruction*)malloc(sizeof(listInstruction)); //metto il (listInstruction*) davanti se no il compilatore fa casino
    if (newListNode == NULL) {
        printf("Errore"); /*Trovare errore piu descrittivo*/
    }
    (*newListNode).headInstruction = NULL;
    
    return newListNode;
}

/*aggiunta di un nuovo nodo alle istruzioni*/
instruction *newInstruction(bool typeFlag, int length) {
    instruction *newInstruction = NULL;
    newInstruction = (instruction*)malloc(sizeof(instruction));
    if (newInstruction == NULL) {
        printf("Errore");/*Trovare errore piu descrittivo*/
    }
    (*newInstruction).typeFlag = typeFlag; /*Assegniamo all' attributo typeFlag della struct il valore passato alla funzione*/
    
    /*Se l' istruzione è bloccante*/
    if (typeFlag) {
        (*newInstruction).length = rand() % length + 1;
    }
    /*Se l' istruzione non è bloccante*/
    if (!typeFlag) {
        (*newInstruction).length = length;
    }
    
    (*newInstruction).next = NULL;
    
    return newInstruction;
}

/*nuovo task*/
task *newTask(int id, int arrival_time) {
    task *newTaskNode = NULL;
    newTaskNode = (task*)malloc(sizeof(task));
    
    if (newTaskNode == NULL) {
        printf("Errore");/*Trovare errore piu descrittivo*/
    }
    
    (*newTaskNode).id = id;
    (*newTaskNode).arrival_time = arrival_time;
    (*newTaskNode).processState = 1;
    (*newTaskNode).programCounter = NULL;
    (*newTaskNode).next = NULL;
    
    /*dato che instr_list è la lista delle istruzini da eseguire
     devo creare un nuovo nodo alla lista delle istruzioni*/
    listInstruction *newListNode = newListInstructionNode();
    (*newTaskNode).instr_list = newListNode;
    
    /*Tentativo diverso dalla roba sopra commentata
     (*newTaskNode).instr_list = newListInstructionNode();
     (*newTaskNode).instr_list -> headInstruction = NULL;
     */
    return newTaskNode;
}

/*nuovo nodo nella coda di task*/
queueTask *newQueueForTask() {
    queueTask *newTaskQueue = NULL;
    newTaskQueue = (queueTask*)malloc(sizeof(queueTask));
    
    if (newTaskQueue == NULL) {
        printf("Errore");/*Trovare errore piu descrittivo*/
    }
    
    (*newTaskQueue).firstTask = NULL;
    
    return newTaskQueue;
}

/*inserisco una istruzione all' interno del task (lista concatenata semplice)
 come paramentri prendo il task nel quale inserire l' istruzione e l' istruzione da metterci*/
void insertInstructionInTask(instruction *instructionToInsert, task *taskToInsertIn) {
    
    instruction *current;
    
    /*creo questa istruzione "instruction" temporanea per usarla come copia dell' istruzione da inserire e poterla inserire*/
    instruction *instructionTmp = (instruction*)malloc(sizeof(instruction));
    (*instructionTmp).typeFlag = (*instructionToInsert).typeFlag;
    (*instructionTmp).length = (*instructionToInsert).length;
    instructionTmp -> next = NULL;
    
    /*se la lista di istruzioni è vuota*/
    if (taskToInsertIn -> instr_list -> headInstruction == NULL) {
        taskToInsertIn -> instr_list -> headInstruction = instructionTmp;
        /*dato che la nuova istruzione è la prima nella lista il PC deve puntare a questa istruzione*/
        (*taskToInsertIn).programCounter = instructionTmp;
    }
    
    /*se la lista non è vuota uso current per iterare su di essa fino a quando non trovo la fine ed inserisco*/
    else {
        current = taskToInsertIn -> instr_list -> headInstruction;
        while ((current -> next) != NULL) {
            current = current -> next;
        }
        current -> next = instructionTmp;
    }
}

/*rimovo l'istruzione in testa alla lista dei task*/
void removeInstructionFromTask(task *taskToRemoveFrom) {
    /*instruction *instructionTmp = (*taskToRemoveFrom).instr_list -> headInstruction;*/
    
    /*se la lista è gia vuota non devo rimuovere niente*/
    /*Esiste un caso in cui questo avviene ?????*/
    if ((*taskToRemoveFrom).instr_list -> headInstruction == NULL) {
        printf("Errore");
    }
    else {
        instruction *toDelete = (*taskToRemoveFrom).instr_list -> headInstruction;
        (*taskToRemoveFrom).instr_list -> headInstruction = (*toDelete).next;
        free (toDelete);
    }
}

/*Inserimento di un task nella queue dei task*/
void insertTaskInQueue(task *taskToInsert, queueTask *queueToInsertIn) {
    /*current task che uso per scorrere la coda e lo inizializzo al primo*/
    task *current;
    
    /*creo un task identico a quello da inserire*/
    task *taskTmp = (task*)malloc(sizeof(task));
    (*taskTmp).id = (*taskToInsert).id;
    (*taskTmp).arrival_time = (*taskToInsert).arrival_time;
    (*taskTmp).processState = (*taskToInsert).processState;
    (*taskTmp).programCounter = (*taskToInsert).programCounter;
    (*taskTmp).instr_list = (*taskToInsert).instr_list;
    (*taskTmp).next = NULL;
    
    /*se la coda è vuota*/
    if ((*queueToInsertIn).firstTask == NULL) {
        (*queueToInsertIn).firstTask = taskTmp;
    }
    else {
        current = (*queueToInsertIn).firstTask;
        /*sposto current finche non arriva alla fine della coda*/
        while ((*current).next != NULL) {
            current = (*current).next;
        }
        (*current).next = taskTmp;
    }
}

/*Rimozione di un task dalla coda dei task*/
void removeTaskFromQueue(queueTask *queueToRemoveFrom) {
    /*Se la coda è vuota*/
    if ((*queueToRemoveFrom).firstTask == NULL) {
        printf("Errore");
    }
    
    task *taskTmp = (*queueToRemoveFrom).firstTask;
    (*queueToRemoveFrom).firstTask = (*queueToRemoveFrom).firstTask -> next;
    free(taskTmp);
}


/*FUNZIONI DI TESTING CANCELARE IN RELEASE*/

/*stampa task*/
void print(task *Task){
    
    instruction *Temp;
    for (Temp = (*Task).instr_list -> headInstruction; Temp != NULL; Temp = (*Temp).next) {
        printf("Istr: %d, %d\n", Temp -> typeFlag, Temp -> length);
    }
}

/*Stampa della coda di task*/
void printAllTask(queueTask *queue) {
    task *temp;
    
    for(temp = (*queue).firstTask; temp != NULL; temp = (*temp).next){
        printf("Task: %d, %d\n", temp->id, temp->arrival_time);
        print(temp);
    }
}
