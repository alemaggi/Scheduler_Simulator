//
//
//  Created by Alessandro Maggi on 31/10/2018.
//  Copyright © 2018 Alessandro Maggi. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include "DataSturcture/list.c"

typedef struct scheduler_info {
    int core;
    FILE *outputFile;
    task *taskList;
    char schedulerType; //usiamo 'p': preemptive, 'n': nonpreemptive
} schedulerInfo;

schedulerInfo *newScheduler(int core, char schedulerType, task *taskList, FILE *outputFile) {
    schedulerInfo *new_scheduler = (schedulerInfo*) malloc(sizeof(schedulerInfo));

    (*new_scheduler).core = core;
    (*new_scheduler).schedulerType = schedulerType;
    (*new_scheduler).taskList = taskList;
    (*new_scheduler).outputFile = outputFile;

    return new_scheduler;
}

void updateSchedulerStatus(FILE *fileName, task *task, int core, int clock) {
    task *status = (*task).processState;
    
    switch(status) {
        case 0:
            fprintf(fileName, "core%d,%d,%d,%s\n", core, clock, task->data.t.id, "new");
            break;
        case 1:
            fprintf(fileName, "core%d,%d,%d,%s\n", core, clock, task->data.t.id, "ready");
            break;
        case 2:
            fprintf(fileName, "core%d,%d,%d,%s\n", core, clock, task->data.t.id, "running");
            break;
        case 3:
            fprintf(fileName, "core%d,%d,%d,%s\n", core, clock, task->data.t.id, "blocked");
            break;
        case 4:
            fprintf(fileName, "core%d,%d,%d,%s\n", core, clock, task->data.t.id, "exit");
            break;
    }

    /*Aggiunta di un task alla coda dei task bloccati*/
    void addTaskToBlockedTaskQueue(task *taskToInsert, queueTask *queue) {
        insertTaskInQueue(taskToInsert, queue)    
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
        for (currentTask = (*queue).firstTask; currentTask != NULL; currentTask = (*currentTask).next) {
            for (currentInstruction = (*instruction).headInstruction; currentInstruction != NULL; currentInstruction = (*currentInstruction).next) {
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
        timeQuantum = abs((durationTimeSum / numberOfInstruction) * (80 / 100)); /*metto abs per sicurezza*/
        return timeQuantum;
    }
}