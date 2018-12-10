#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <getopt.h>
#include <string.h>

#ifndef LIST
#define LIST

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
    int processState; /*usiamo un numero per rappresentare i 5 stati 0:new, 1:ready, 2: running, 3:blocked, 4:exit*/
    task *next;
    int blockTime;
    int sommaLunghezzaIstruzioni;
};

/*Struttura per rappresentare una istruzione*/
struct Instruction {
    bool typeFlag; //1: bloccante, 0:nonBloccante
    int length;
    int executionTime;
    instruction *next;
};

/*Struttura per rappresentare la lista di istruzioni che contiene solo un puntatore all' istruzione successiva*/
struct List_for_instruction {
    instruction *headInstruction;
};

/*Struttura per rappresentare la lista di istruzioni che contiene solo un puntatore all' istruzione successiva*/
struct Queue_for_task {
    task *firstTask;
    int size; //mi serve poi per potrene controllare la presenza di elementi
};

/*nuova lista di istruzioni*/
listInstruction *newListInstructionNode() {
    listInstruction *newListNode = NULL;
    newListNode = (listInstruction*)malloc(sizeof(listInstruction)); 
    if (newListNode == NULL) {
        printf("Errore in newListInstructionNode");
    }
    (*newListNode).headInstruction = NULL;
    
    return newListNode;
}

/*aggiunta di un nuovo nodo alle istruzioni*/
instruction *newInstruction(bool typeFlag, int length) {
    instruction *newInstruction = NULL;
    newInstruction = (instruction*)malloc(sizeof(instruction));
    if (newInstruction == NULL) {
        printf("Errore in newInstruction");
    }
    (*newInstruction).typeFlag = typeFlag; /*Assegniamo all' attributo typeFlag della struct il valore passato alla funzione*/
    (*newInstruction).executionTime = length;
    /*Se l' istruzione è bloccante*/
    if (typeFlag == true) {
        (*newInstruction).length = rand() % length + 1;
    }
    /*Se l' istruzione non è bloccante*/
    if (typeFlag == false) {
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
        printf("Errore in newTask");
    }
    
    (*newTaskNode).id = id;
    (*newTaskNode).arrival_time = arrival_time;
    (*newTaskNode).processState = 0;
    (*newTaskNode).programCounter = NULL;
    (*newTaskNode).next = NULL;
    (*newTaskNode).blockTime = 0;
    (*newTaskNode).sommaLunghezzaIstruzioni = 0;
    
    (*newTaskNode).instr_list = newListInstructionNode();
     
    return newTaskNode;
}

/*nuovo nodo nella coda di task*/
queueTask *newQueueForTask() {
    queueTask *newTaskQueue = NULL;
    newTaskQueue = (queueTask*)malloc(sizeof(queueTask));
    
    if (newTaskQueue == NULL) {
        printf("Errore in newQueueForTask");
    }
    
    (*newTaskQueue).firstTask = NULL;
    (*newTaskQueue).size = 0;

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
    (*instructionTmp).executionTime = (*instructionToInsert).executionTime;
    (*instructionTmp).next = NULL;
    
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
        if ((*taskToRemoveFrom).instr_list -> headInstruction == NULL) {
        printf("Errore in removeInstructionFromTask");
    }
    else {
        instruction* toDelete = (instruction*)malloc(sizeof(instruction));
        toDelete = (*taskToRemoveFrom).instr_list -> headInstruction;
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
    (*taskTmp).blockTime = (*taskToInsert).blockTime;
    (*taskTmp).sommaLunghezzaIstruzioni = (*taskToInsert).sommaLunghezzaIstruzioni;
    
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
    (*queueToInsertIn).size++;
}

/*Rimozione di un task dalla coda dei task*/
void removeTaskFromQueue(queueTask *queueToRemoveFrom) {
    if ((*queueToRemoveFrom).firstTask == NULL) {
        printf("Errore in removeTaskFromQueue");
    }
    
    task* taskTmp = (task*)malloc(sizeof(task));
    taskTmp = (*queueToRemoveFrom).firstTask;

    (*queueToRemoveFrom).firstTask = (*queueToRemoveFrom).firstTask -> next;
    free(taskTmp);
    (*queueToRemoveFrom).size--;
}

/*Funzione per controllare se una coda è vuota*/
int isEmpty(queueTask* Queue) {
    if (Queue == NULL) {
        return 0;
    }
    if (Queue->size == 0) {
        return 1;
    } else {
        return 0;
    }
}

#endif