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
    //prendo i parametri dello scheduler
    int core = ((schedulerInfo *)schedInfo)-> core;
    char schedulerType = ((schedulerInfo *)schedInfo) -> schedulerType;
    queueTask *taskList = ((schedulerInfo *)schedInfo) -> taskList;
    char *outputFile = ((schedulerInfo *)schedInfo) -> outputFile;

    //definisco il clock della simulazione
    int clock = 1;

    //Uno scheduler NON deve mandare in esecuzione un task se: ​clock(core) < clock(task->arrival_time)
    while (clock < (((schedulerInfo *)schedInfo) -> taskList -> firstTask -> arrival_time)) {
        clock++;
    }

    //FCFS --> nonpreemptive
    if (schedulerType == 'n') {

        queueTask *blockedTask = newQueueForTask();
        queueTask *readyTask = newQueueForTask();
        
        bool done = false;

        while (!done) {
            if (!isEmpty(taskList)) {
                insertTaskInQueue((*taskList).firstTask, readyTask);
                updateSchedulerStatus(outputFile, (*readyTask).firstTask, core, clock);
                //cambio il process state a 1 (ready)
                (*readyTask).firstTask -> processState = 1;
                updateSchedulerStatus(outputFile, (*readyTask).firstTask, core, clock);
                //rimuovo il task dalla task list
                removeTaskFromQueue(taskList);
            }

            if (!isEmpty(readyTask)) {
                instruction* currentInstruction = (*readyTask).firstTask -> instr_list -> headInstruction;

                //eseguo le istruzioni del task finche non le finisco 
                while ((*currentInstruction).next != NULL) {
                    //istruzione non bloccante
                    if ((*currentInstruction).typeFlag == 0) {
                        (*readyTask).firstTask -> processState = 2; //running
                        updateSchedulerStatus(outputFile, (*readyTask).firstTask, core, clock);
                        //devo far aumentare il clock della durata dell' istruzione
                        int instructionEndClock = clock + (*currentInstruction).length;
                        while (clock < instructionEndClock) {
                            clock++;
                        }
                        //se l'istruzione eseguita era l'ultima del task lo mando in exit
                        if ((*currentInstruction).next == NULL) {
                            (*readyTask).firstTask -> processState = 4;
                            updateSchedulerStatus(outputFile, (*readyTask).firstTask, core, clock);
                        }
                        removeInstructionFromTask(readyTask);
                    }
                    //istruzione bloccante
                    else {
                        (*readyTask).firstTask -> processState = 3;
                        updateSchedulerStatus(outputFile, (*readyTask).firstTask, core, clock);
                        (*readyTask).firstTask ->clockWhenBlocked = clock;
                        insertTaskInQueue((*readyTask).firstTask, blockedTask);
                        removeTaskFromQueue(readyTask);
                    }
                    currentInstruction = (*currentInstruction).next;
                }   
            }

            if (!isEmpty(blockedTask)) {
                if ((*blockedTask).firstTask -> clockWhenBlocked + 
                (*blockedTask).firstTask -> instr_list -> headInstruction -> length <= clock) {
                    (*blockedTask).firstTask -> processState = 1;
                    updateSchedulerStatus(outputFile, (*blockedTask).firstTask, core, clock);
                    insertTaskInQueue((*blockedTask).firstTask, readyTask);
                    removeInstructionFromTask((*blockedTask).firstTask); //tolgo l'istruzione che mi aveva blocccato
                    removeTaskFromQueue(blockedTask);
                }
            }

            clock ++;

            if (isEmpty(readyTask) && isEmpty(blockedTask) && isEmpty(taskList)) {
                done = true;
            }
        }
    }

    //RR --> preemptive
    if (schedulerType == 'p') {
        int timeQuantum = calculateTimeQuantum(taskList);

        queueTask *readyTask = newQueueForTask();
        queueTask *blockedTask = newQueueForTask();

        bool done = false;

        while (!done) {
            if (!isEmpty(taskList)) {
                insertTaskInQueue((*taskList).firstTask, readyTask);
                updateSchedulerStatus(outputFile, (*readyTask).firstTask, core, clock);
                //cambio il process state a 1 (ready)
                (*readyTask).firstTask -> processState = 1;
                updateSchedulerStatus(outputFile, (*readyTask).firstTask, core, clock);
                //rimuovo il task dalla task list
                removeTaskFromQueue(taskList);
            }

            if (!isEmpty(readyTask)) {
                instruction *currentInstruction = (*readyTask).firstTask -> instr_list -> headInstruction;
                
                //se l'istruzione non è bloccante
                if ((*currentInstruction).typeFlag == 0) {
                    //mando in running
                    (*readyTask).firstTask -> processState = 2;
                    updateSchedulerStatus(outputFile, (*readyTask).firstTask, core, clock);
                    //se l'istruzione è piu corta del quanto di tempo
                    if ((*currentInstruction).length <= timeQuantum) {

                        removeInstructionFromTask(readyTask);
                        //faccio aumentare il clock fino ad esaurire il quanto (GIUSTO ???)
                        while (clock < (*readyTask).firstTask -> instr_list -> headInstruction -> length + timeQuantum) {
                            clock++;
                        }
                        //se l'istruzione è l'ultima del task lo mando in exit
                        if ((*currentInstruction).next == NULL) {
                            (*readyTask).firstTask -> processState = 4;
                            updateSchedulerStatus(outputFile, (*readyTask).firstTask, core, clock);
                            removeInstructionFromTask((*readyTask).firstTask);
                            removeTaskFromQueue(readyTask);
                        }
                    }

                    //se l'istruzione è piu lunga del task
                    if ((*currentInstruction).length > timeQuantum) {
                        //aggiorno la lunghezza dell' istruzione 
                        (*readyTask).firstTask -> instr_list -> headInstruction -> length -= timeQuantum; 
                        //faccio aumentare il clock fino ad esaurire il quanto (GIUSTO ???)
                        while (clock < (*readyTask).firstTask -> instr_list -> headInstruction -> length + timeQuantum) {
                            clock++;
                        }
                        //la tolgo da running e la metto in coda ai ready
                        (*readyTask).firstTask -> processState = 1;
                        updateSchedulerStatus(outputFile, (*readyTask).firstTask, core, clock);
                        insertTaskInQueue((*readyTask).firstTask, readyTask);
                        removeTaskFromQueue(readyTask);
                    }
                }

                //istruzione bloccante
                if ((*currentInstruction).typeFlag == 1) {
                    (*readyTask).firstTask -> processState = 4;
                    updateSchedulerStatus(outputFile, (*readyTask).firstTask, core, clock);
                    (*readyTask).firstTask -> clockWhenBlocked = clock;
                    insertTaskInQueue((*readyTask).firstTask, blockedTask);
                    removeTaskFromQueue(readyTask);
                } 
            }

            if (!isEmpty(blockedTask)) {
                if ((*blockedTask).firstTask -> clockWhenBlocked + 
                (*blockedTask).firstTask -> instr_list -> headInstruction -> length <= clock) {
                    (*blockedTask).firstTask -> processState = 1;
                    updateSchedulerStatus(outputFile, (*blockedTask).firstTask, core, clock);
                    insertTaskInQueue((*blockedTask).firstTask, readyTask);
                    removeInstructionFromTask((*blockedTask).firstTask); //tolgo l'istruzione che mi aveva blocccato
                    removeTaskFromQueue(blockedTask);
                }
            }

            clock ++;

            if (isEmpty(readyTask) && isEmpty(blockedTask) && isEmpty(taskList)) {
                done = true;
            }
        }
    }
}