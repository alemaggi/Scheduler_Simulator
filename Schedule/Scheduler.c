//
//
//  Created by Alessandro Maggi on 31/10/2018.
//  Copyright © 2018 Alessandro Maggi. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "./../DataStructure/list.c"

/*CADDU CONTROLLA LE MUTEX CHE IO NON SO QUANDO VADANO USATE*/

typedef struct scheduler_info {
    int core;
    char *outputFile;
    queueTask *taskList;
    char schedulerType; //usiamo 'p': preemptive, 'n': nonpreemptive
} schedulerInfo;

//Questa funzione qui potrebbe essere supreflua
schedulerInfo *newScheduler(int core, char schedulerType, queueTask *taskList, char *outputFile) {
    schedulerInfo *new_scheduler = (schedulerInfo*) malloc(sizeof(schedulerInfo));

    (*new_scheduler).core = core;
    (*new_scheduler).schedulerType = schedulerType;
    (*new_scheduler).taskList = taskList;
    (*new_scheduler).outputFile = outputFile;

    return new_scheduler;
}

void updateSchedulerStatus(char *fileName, task *task, int core, int clock) {
    
    FILE* outputFile = fopen(fileName, "a");

    switch((*task).processState) {
        case 0:
            fprintf(outputFile, "core%d,%d,%d,%s\n", core, clock, (*task).id, "new");
            break;
        case 1:
            fprintf(outputFile, "core%d,%d,%d,%s\n", core, clock, (*task).id, "ready");
            break;
        case 2:
            fprintf(outputFile, "core%d,%d,%d,%s\n", core, clock, (*task).id, "running");
            break;
        case 3:
            fprintf(outputFile, "core%d,%d,%d,%s\n", core, clock, (*task).id, "blocked");
            break;
        case 4:
            fprintf(outputFile, "core%d,%d,%d,%s\n", core, clock, (*task).id, "exit");
            break;
    }
}


/*Aggiunta di un task alla coda dei task bloccati*/
void addTaskToBlockedTaskQueue(task *taskToInsert, queueTask *queue) {
    insertTaskInQueue(taskToInsert, queue);  
}

/*Aggiunta di un task alla coda dei task ready*/
void addTaskToReadyTaskQueue(task *taskToInsert, queueTask *queue) {
    insertTaskInQueue(taskToInsert, queue);
}

/*Calcolo del quanto di tempo in modo che sia minore dell' 80% del tempo in cui 
la cpu viene impiegata senza istruzioni di I/O */
int calculateTimeQuantum(queueTask *queue) {
    int numberOfInstruction = 0;
    int durationTimeSum = 0;
    int timeQuantum = 0;
    /*devo scorrere tutte le istruzioni all' interno di ogni task nella coda:
    controllo l'istruzione è bloccante non la considero nella somma*/
    for (task *currentTask = (*queue).firstTask; currentTask != NULL; currentTask = (*currentTask).next) {
        for (instruction *currentInstruction = (*currentTask).instr_list -> headInstruction ; currentInstruction != NULL; currentInstruction = (*currentInstruction).next) {
            /*Se l'istruzione è bloccante allora non la considero*/
            if ((*currentInstruction).typeFlag == true) {
                continue;
            }
            /*Se l' istruzione non è bloccante*/
            else {
                numberOfInstruction ++;
                durationTimeSum += (*currentInstruction).length;
            }
        }
    }
    timeQuantum = ((durationTimeSum * 0.8) / numberOfInstruction); 
    return timeQuantum;
}

/*Mutex*/
/*Andrebbe usata ogni volta che scrivi o leggi su una coda*/
pthread_mutex_t mutexOne = PTHREAD_MUTEX_INITIALIZER;

/*Funzione di scheduling*/
void *changeProcessState(void *schedInfo) {

    //parametri dello schedInfouler
    int core = ((schedulerInfo *)schedInfo)-> core;
    char schedulerType = ((schedulerInfo *)schedInfo) -> schedulerType;
    queueTask *taskList = ((schedulerInfo *)schedInfo) -> taskList;
    char *outputFile = ((schedulerInfo *)schedInfo) -> outputFile;

    //definisco il clock della simulazione
    int clock = 1;

    /*NON SONO SICURO VADA BENE E VADA MESSO QUI*/
    //Uno scheduler NON deve mandare in esecuzione un task se: ​clock(core) < clock(task->arrival_time)
    while (clock < (((schedulerInfo *)schedInfo) -> taskList -> firstTask -> arrival_time)) {
        clock++;
    }

    //Algoritmo Round Robin
    if (schedulerType == 'r') {
        
        int timeQuantum = calculateTimeQuantum(taskList);

        //inizializzo la coda dei bloccati
        queueTask *blockedTaskQueue = newQueueForTask();
        //inizializzo la coda dei ready
        queueTask *readyTaskQueue = newQueueForTask();

        //variabile booleana per farmi restare dentro al ciclo finche le code dei ready e dei blocked non sono vuote
        bool work = true;

        //boolean per togliere i task da taskList finche non è vuota
        bool keepinsertingTaskInReady = true;

        /*Metto i task di taskList in Ready finche non è vuota*/
        while (keepinsertingTaskInReady) {
            pthread_mutex_lock(&mutexOne);
            if (!isEmpty(taskList)) {
                //se i task sono a new(0) li metti in ready
                if ((*taskList).firstTask -> processState == 0) { //probabilmente è superfluo

                    //stampo che i task sono a new (Deve succedere solo una volta)
                    updateSchedulerStatus(outputFile, (*taskList).firstTask, core, clock);

                    //inserisco i task nella coda ready
                    insertTaskInQueue((*taskList).firstTask, readyTaskQueue);
                    //cambio il process state a READY(1)
                    (*readyTaskQueue).firstTask -> processState = 1;
                    //stampo il cambiamento di stato
                    updateSchedulerStatus(outputFile, (*readyTaskQueue).firstTask, core, clock);
                    removeTaskFromQueue(taskList);
                }
            }
            pthread_mutex_unlock(&mutexOne);
            if (isEmpty(taskList)) {
                keepinsertingTaskInReady = false;
            }
        }

        while (work == true) {
            
            pthread_mutex_lock(&mutexOne);
            /*Se la coda dei bloccati non è vuota*/
            if (!isEmpty(blockedTaskQueue)) {
                /*Se c'è un task che posso sbloccare*/
                if (((blockedTaskQueue -> firstTask -> instr_list -> headInstruction -> length) + //non sono sicuro sia giusto
                (blockedTaskQueue -> firstTask -> arrival_time)) >= clock) {
                
                    //cambio il suo process state a READEY(1)
                    (*blockedTaskQueue).firstTask -> processState = 1;
                    //lo metto nella coda ready
                    insertTaskInQueue((*blockedTaskQueue).firstTask, readyTaskQueue);
                    //lo rimuovo dalla coda dei bloccati
                    removeTaskFromQueue(blockedTaskQueue);
                    //stampo il cambiamento di stato
                    updateSchedulerStatus(outputFile, (*blockedTaskQueue).firstTask, core, clock);
                }
            }
            pthread_mutex_unlock(&mutexOne);

            pthread_mutex_lock(&mutexOne);
            /*Se la coda dei ready non è vuota allora*/
            if (!isEmpty(readyTaskQueue)) {
                /*Se l'istruzione non è bloccante*/
                if ((*readyTaskQueue).firstTask -> instr_list -> headInstruction -> typeFlag == 0) {
                    //cambio lo stato a running(2)
                    (*readyTaskQueue).firstTask -> processState = 2;
                    updateSchedulerStatus(outputFile, (*readyTaskQueue).firstTask, core, clock);

                    int instructionEnd = (*readyTaskQueue).firstTask -> instr_list -> headInstruction -> length + clock;
                    
                    /*Se la length dell' istruzione è minore del quanto*/
                    if ((*readyTaskQueue).firstTask -> instr_list -> headInstruction -> length <= timeQuantum) {

                        while (clock != instructionEnd) {
                        clock++;
                        }
                        
                        //tolgo l'istruzione dal task
                        removeInstructionFromTask((*readyTaskQueue).firstTask);
                        //se l'istruzione fosse stata l' ultima del task mando il task in exit
                        if ((*readyTaskQueue).firstTask -> instr_list -> headInstruction -> next == NULL) {
                            (*readyTaskQueue).firstTask -> processState = 4;
                            updateSchedulerStatus(outputFile, (*readyTaskQueue).firstTask, core, clock);
                            //rimuovo il task dalla coda
                            removeTaskFromQueue(readyTaskQueue);                            
                        }
                    }
                    /*Se la length dell' istruzione è maggiore del quanto*/
                    if ((*readyTaskQueue).firstTask -> instr_list -> headInstruction -> length > timeQuantum) {
                        
                        //aggiorno la lunghezza dell'istruzione
                        (*readyTaskQueue).firstTask -> instr_list -> headInstruction -> length -= timeQuantum;

                        while (clock != clock + timeQuantum) {
                        clock++;
                        }
                        //mando il task in fondo alla coda ready
                        insertTaskInQueue(readyTaskQueue -> firstTask, readyTaskQueue);
                        removeTaskFromQueue(readyTaskQueue);
                        //cambio lo stato e lo loggo
                        (*readyTaskQueue).firstTask -> processState = 1;
                        updateSchedulerStatus(outputFile, (*readyTaskQueue).firstTask, core, clock);
                    }
                }

                /*Se l'istruzione è bloccante*/
                if ((*readyTaskQueue).firstTask -> instr_list -> headInstruction -> typeFlag == 1) {
                    //mando il task nella coda dei bloccati
                    insertTaskInQueue((*readyTaskQueue).firstTask, blockedTaskQueue);
                    (*readyTaskQueue).firstTask -> processState = 3;
                    //loggo il cambiamento di stato
                    updateSchedulerStatus(outputFile, (*readyTaskQueue).firstTask, core, clock);
                    //rimuovo il task dalla coda dei ready
                    removeTaskFromQueue(readyTaskQueue);
                }
            }
            pthread_mutex_unlock(&mutexOne);

            //aggiorno il clock
            clock++;

            pthread_mutex_lock(&mutexOne);
            //se tutte le code sono vuote cambio work e non rientro nel ciclo
            if (isEmpty(blockedTaskQueue) && isEmpty(readyTaskQueue)) {
                work = false;
            }
            pthread_mutex_unlock(&mutexOne);
        }
    }

    //caso FCFS
    /*I task vengono gestiti tramite coda FIFO*/
    if (schedulerType == 'n') {
        //inizializzo la coda dei bloccati
        queueTask *blockedTaskQueue = newQueueForTask();
        //inizializzo la coda dei ready
        queueTask *readyTaskQueue = newQueueForTask();

        //variabile booleana per farmi restare dentro al ciclo finche le code ready e blocked non sono vuote
        bool work = true;

        //boolena per inserire task in ready finche non è vuota la task list
        bool keepinsertingTaskInReady = true;

        while (keepinsertingTaskInReady) {
            pthread_mutex_lock(&mutexOne);
            if (!isEmpty(taskList)) {
                //se i task sono a new(0) li metti in ready
                if ((*taskList).firstTask -> processState == 0) { //questo controllo dovrebbe essere inutile
                    //inserisco i task nella coda ready
                    insertTaskInQueue((*taskList).firstTask, readyTaskQueue);
                    //cambio il process state a READY(1)
                    (*readyTaskQueue).firstTask -> processState = 1;
                    //stampo il cambiamento di stato
                    updateSchedulerStatus(outputFile, (*readyTaskQueue).firstTask, core, clock);
                    removeTaskFromQueue(taskList);
                }
            }
            pthread_mutex_unlock(&mutexOne);
            
            pthread_mutex_lock(&mutexOne);
            if (isEmpty(taskList)) {
                keepinsertingTaskInReady = false;
            }
            pthread_mutex_unlock(&mutexOne);   
        }

        while (work == true) {
            
            pthread_mutex_lock(&mutexOne);
            /*Se la coda dei bloccati non è vuota*/
            if (!isEmpty(blockedTaskQueue)) {
                /*Se c'è un task che posso sbloccare*/
                if (((blockedTaskQueue -> firstTask -> instr_list -> headInstruction -> length) + //non sono sicuro sia giusto
                (blockedTaskQueue -> firstTask -> arrival_time)) >= clock) {
                
                    //cambio il suo process state a READEY(1)
                    (*blockedTaskQueue).firstTask -> processState = 1;
                    //lo metto nella coda ready
                    insertTaskInQueue((*blockedTaskQueue).firstTask, readyTaskQueue);
                    //lo rimuovo dalla coda dei bloccati
                    removeTaskFromQueue(blockedTaskQueue);
                    //stampo il cambiamento di stato
                    updateSchedulerStatus(outputFile, (*blockedTaskQueue).firstTask, core, clock);
                }
            }
            pthread_mutex_unlock(&mutexOne);

            //se la coda dei ready non è vuota
            pthread_mutex_lock(&mutexOne);
            if (!isEmpty(readyTaskQueue)) {
                //se l' istruzione non è bloccante
                if ((*readyTaskQueue).firstTask -> instr_list -> headInstruction -> typeFlag == 0) {
                    //cambio lo stato a running
                    (*readyTaskQueue).firstTask -> processState = 2;
                    //faccio aumentare il clock della durata dell' istruzione
                    int endTime = clock + (*readyTaskQueue).firstTask -> instr_list -> headInstruction -> length;
                    while (clock <= endTime) {
                        clock++;
                    }
                    //se l' istruzione era l'ultima del task lo mando in exit
                    if ((*readyTaskQueue).firstTask -> next == NULL) {
                        (*readyTaskQueue).firstTask -> processState = 4;
                        updateSchedulerStatus(outputFile, (*readyTaskQueue).firstTask, core, clock);
                        removeTaskFromQueue(readyTaskQueue);
                    }
                }
                //se l'istruzione è bloccante
                if ((*taskList).firstTask -> instr_list -> headInstruction -> typeFlag == 1) {
                    //cambio lo stato a blocked e metto il task nei blocked
                    (*taskList).firstTask -> processState = 3;
                    insertTaskInQueue((*taskList).firstTask, blockedTaskQueue);
                    //rimuovo il task dai ready
                    removeTaskFromQueue(readyTaskQueue);
                    //loggo il cambiamento
                    updateSchedulerStatus(outputFile, (*taskList).firstTask, core, clock);
                }   
            }
            pthread_mutex_unlock(&mutexOne);

            pthread_mutex_lock(&mutexOne);
            //se tutte le code sono vuote cambio work e non rientro nel ciclo
            if (isEmpty(blockedTaskQueue) && isEmpty(readyTaskQueue)) {
                work = false;
            }
            pthread_mutex_unlock(&mutexOne);
        }
    }   
}