#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "list.c"

pthread_mutex_t mutexOne = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexTwo = PTHREAD_MUTEX_INITIALIZER;

typedef struct schedulerInfo {
    int core;
    char *outputFile;
    queueTask *taskList;
    bool schedulerType; //usiamo 0: preemptive, 1: nonpreemptive
} schedulerInfo_t;

schedulerInfo_t *newScheduler(int core,  char *outputFile, queueTask *taskList, bool schedulerType){
    
    schedulerInfo_t *new_scheduler = (schedulerInfo_t*)malloc(sizeof(schedulerInfo_t));

    new_scheduler->core = core;
    new_scheduler->outputFile = outputFile;
    new_scheduler->taskList = taskList;
    new_scheduler->schedulerType = schedulerType;

    return new_scheduler;
}

void printSchedulerStatus(char *fileName, task *task, int core, int clock){
    
    FILE* outputFile = fopen(fileName, "a"); //lo prendo in append

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

    fclose(outputFile);
}

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

void* schedule(void* schedInfo){

    schedInfo = newScheduler(((schedulerInfo_t*)schedInfo)->core, ((schedulerInfo_t*)schedInfo)->outputFile, ((schedulerInfo_t*)schedInfo)->taskList, ((schedulerInfo_t*)schedInfo)->schedulerType);

    //DA CANCELLARE A MENO CHE NON FUNZIONI LA ROBA SOTTO COMMENTATA
    int Core = ((schedulerInfo_t*) schedInfo)->core;
    char *OutPut = ((schedulerInfo_t*) schedInfo)->outputFile;
    queueTask *TaskList = ((schedulerInfo_t*) schedInfo)->taskList;
    bool SchedulerType = ((schedulerInfo_t*) schedInfo)->schedulerType;

    //schedulerInfo_t* schedInfo = newScheduler(Core, OutPut, TaskList, SchedulerType);


    int clock = 1;

    bool done = false;

    pthread_mutex_lock(&mutexOne);
    //NON SONO SICURO DI QUESTO
    while (((schedulerInfo_t*)schedInfo) -> taskList -> firstTask -> arrival_time != clock){
        clock++;
    }
    pthread_mutex_unlock(&mutexOne);

    //FCFS
    if ((((schedulerInfo_t*)schedInfo)->schedulerType) == true) {
        queueTask* blockedTask = newQueueForTask();

        while (!done){
            queueTask* readyTask = newQueueForTask();
             pthread_mutex_lock(&mutexOne);
            if (!isEmpty(((schedulerInfo_t*)schedInfo)->taskList)){
                // metto in ready loggo l'arrivo a new
                insertTaskInQueue(((schedulerInfo_t*)schedInfo)->taskList->firstTask, readyTask);
                printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*readyTask).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                
                //metto in ready e loggo il cambiamento
                (*readyTask).firstTask->processState = 1;
                printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*readyTask).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);

                //preparo il nuovo task
                ((schedulerInfo_t*)schedInfo)->taskList->firstTask = ((schedulerInfo_t*)schedInfo)->taskList->firstTask->next;
            }
            else if (!isEmpty(blockedTask)){
                if ((*blockedTask).firstTask->clockWhenBlocked +  (*blockedTask).firstTask -> instr_list -> headInstruction -> length <= clock){
                    (*blockedTask).firstTask -> processState = 1;
                    printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*blockedTask).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                    //faccio diventare non bloccante l'istruzione che mi ha bloccato
                    (*blockedTask).firstTask->instr_list->headInstruction->typeFlag = 0; //non bloccante
                    (*blockedTask).firstTask->instr_list->headInstruction->length = (*blockedTask).firstTask->instr_list->headInstruction->executionTime; //la faccio eseguire per la lunghezza e non per il tempo che è stata bloccata
                    //la metto nei ready
                    insertTaskInQueue((*blockedTask).firstTask, readyTask);
                    //tolgo dai blocked
                    removeTaskFromQueue(readyTask);
            
                }
            }
            pthread_mutex_unlock(&mutexOne);

            if (!isEmpty(readyTask)){
                pthread_mutex_lock(&mutexOne);
                instruction *currentInstruction = (*readyTask).firstTask->instr_list->headInstruction;
                pthread_mutex_unlock(&mutexOne);
                
                while (currentInstruction->next != NULL){
                    //istruzione non bloccante
                    if (currentInstruction->typeFlag == 0){
                        (*readyTask).firstTask->processState = 2; //running
                        printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*readyTask).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);

                        int durata = currentInstruction->length + clock;
                        while (clock != durata){
                            clock++;
                        }

                        //se l'istruzione è l'ultima del task
                        if (currentInstruction->next == NULL) {
                            (*readyTask).firstTask->processState = 4;
                            printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*readyTask).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                        }
                    }
                    else {
                        (*readyTask).firstTask->processState = 3;
                        printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*readyTask).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);

                        (*readyTask).firstTask->clockWhenBlocked = clock;

                        insertTaskInQueue((*readyTask).firstTask, blockedTask);
                        break;
                    }

                    removeInstructionFromTask((*readyTask).firstTask);

                    currentInstruction = currentInstruction->next;
                }
            }

            clock++;
            pthread_mutex_lock(&mutexOne);
            if (isEmpty(readyTask) && isEmpty(blockedTask) && isEmpty(((schedulerInfo_t*)schedInfo)->taskList)){
                done = true;
            }
            pthread_mutex_unlock(&mutexOne);
        }
    }

    //RoundRobin
    else {
        int timeQuantum = calculateTimeQuantum(((schedulerInfo_t*)schedInfo)->taskList);

        queueTask* readyTask = newQueueForTask();
        queueTask* blockedTask = newQueueForTask();

        while (!done) {
            
            pthread_mutex_lock(&mutexOne);
            if (!isEmpty(((schedulerInfo_t*)schedInfo)->taskList)) {
                //loggo l' arrivo a new
                printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, ((schedulerInfo_t*)schedInfo)->taskList->firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                //lo metto nei ready
                insertTaskInQueue(((schedulerInfo_t*)schedInfo)->taskList->firstTask, readyTask);
                //cambio lo stato a ready e loggo il cambiamento
                ((schedulerInfo_t*)schedInfo)->taskList->firstTask->processState = 1;
                printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, ((schedulerInfo_t*)schedInfo)->taskList->firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);

                ((schedulerInfo_t*)schedInfo)->taskList->firstTask = ((schedulerInfo_t*)schedInfo)->taskList->firstTask->next;
            }
            else if (!isEmpty(blockedTask)) {

                if ((*blockedTask).firstTask->clockWhenBlocked +  (*blockedTask).firstTask -> instr_list -> headInstruction -> length <= clock){
                    (*blockedTask).firstTask -> processState = 1;
                    printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*blockedTask).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                    //faccio diventare non bloccante l'istruzione che mi ha bloccato
                    (*blockedTask).firstTask->instr_list->headInstruction->typeFlag = 0; //non bloccante
                    (*blockedTask).firstTask->instr_list->headInstruction->length = (*blockedTask).firstTask->instr_list->headInstruction->executionTime; //la faccio eseguire per la lunghezza e non per il tempo che è stata bloccata
                    //la metto nei ready
                    insertTaskInQueue((*blockedTask).firstTask, readyTask);
                    //tolgo dai blocked
                    removeTaskFromQueue(readyTask);
                }
            }
            pthread_mutex_unlock(&mutexOne);

            if (!isEmpty(readyTask)){
                pthread_mutex_lock(&mutexOne);
                instruction* currentInstruction = readyTask->firstTask->instr_list->headInstruction;
                pthread_mutex_unlock(&mutexOne);

                //istruzione non bloccante
                if (currentInstruction->typeFlag == 0){
                    //mando in running e loggo
                    readyTask->firstTask->processState = 2;
                    printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, readyTask->firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                    //se l'istruzione sta tuttta dentro il quanto
                    if (currentInstruction->length <= timeQuantum) {
                        //faccio aumentare il clock della durata dell'istruzione
                        int endTime = clock + currentInstruction->length;
                        //tolgo l'istruzione dal task
                        removeInstructionFromTask(readyTask->firstTask); //non sono sicuro vada bene
                        while (clock != endTime){
                            clock++;
                        }
                        //Se l'istruzione è l'ultima del task
                        if (currentInstruction->next == NULL){
                            //mando in exit e loggo
                            readyTask->firstTask->processState = 4;
                            printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, readyTask->firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                            //tolgo il task dalla coda
                            removeTaskFromQueue(readyTask);
                        }
                    }
                    if (currentInstruction->length > timeQuantum) {
                        //accorcio la lunghezza dell'istruzione del quanto
                        currentInstruction->length = currentInstruction->length - timeQuantum;
                        //la metto nei blocked
                        insertTaskInQueue((*readyTask).firstTask, blockedTask);
                        //cambio lo stato e lo loggo
                        (*readyTask).firstTask->processState = 3;
                        printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*readyTask).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                    }
                }

                //istruzione bloccante
                else{
                    //mando in running e loggo
                    readyTask->firstTask->processState = 2;
                    printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, readyTask->firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                    //mando in blocked e loggo
                    readyTask->firstTask->processState = 3;
                    insertTaskInQueue((*readyTask).firstTask, blockedTask);
                    printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, readyTask->firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                }
            }

            clock++;

            pthread_mutex_lock(&mutexOne);
            if (isEmpty(readyTask) && isEmpty(blockedTask) && isEmpty(((schedulerInfo_t*)schedInfo)->taskList)){
                done = true;
            }
            pthread_mutex_unlock(&mutexOne);
        }
    }

    return NULL;
}

void schedulate(queueTask *Queue, char *fileName, bool schedulerType) {
    int coreNumber = 2;
    
    pthread_t thread[coreNumber];

    schedulerInfo_t Scheduler[coreNumber];

    //creazione parametri per il thread
    for (int i = 0; i < coreNumber; i++) {
        Scheduler[i].outputFile = fileName;
        Scheduler[i].schedulerType = schedulerType;
        Scheduler[i].taskList = Queue;
        Scheduler[i].core = i;

        //se non riesce a creare il thread
        if (pthread_create(&thread[i], NULL, &schedule, &Scheduler[i]) != 0) {
            exit(1); //trovare di meglio
        }
    }
    
    for (int i = 0; i < coreNumber; i++) {
        pthread_join(thread[i], NULL);
    }
}
