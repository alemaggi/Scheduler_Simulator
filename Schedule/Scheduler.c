#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <getopt.h>
#include <string.h>

#include "list.c"

pthread_mutex_t mutexOne = PTHREAD_MUTEX_INITIALIZER;

typedef struct schedulerInfo {
    int core;
    char *outputFile;
    queueTask *taskList;
    bool schedulerType; //usiamo false: preemptive (RR), true: nonpreemptive (SPN)
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


/*QUICK SORT INIZIO----------------------------------------------------------*/

//metodo per sommare le lunghezze delle istruzioni di ogni taske assegnarlo come attributo
void calcosaSommaIstruzioniTask(queueTask* queue){
    for (task *currentTask = (*queue).firstTask; currentTask != NULL; currentTask = (*currentTask).next) {
        int somma = 0;
        for (instruction* current = currentTask->instr_list->headInstruction; current != NULL; current = current->next){
            somma += current->length;
        }
        currentTask->sommaLunghezzaIstruzioni = somma;
    }    
}


//return the last element of a list (posso sostituirlo con un attributo nella struttura)
task* getTail(task* current){
    while (current != NULL && (*current).next != NULL){
        current = current->next;
    }
    return current;
}


//partitions the list taking the last element as the pivot
task* partition(task* head, task* end, task** newHead, task** newEnd){
    task* pivot = end;
    task* prev = NULL;
    task* cur = head; 
    task* tail = pivot;

    // During partition, both the head and end of the list might change 
    // which is updated in the newHead and newEnd variables
    while (cur != pivot){
        if (cur->sommaLunghezzaIstruzioni < pivot->sommaLunghezzaIstruzioni){
            // First node that has a value less than the pivot becomes the new head 
            if ((*newHead) == NULL){
                (*newHead) = cur;
            }
            prev = cur;
            cur = cur->next;
        }
        //if the instruction' s length is greater than pivot's length
        else{
            //move our instruction to next of tail, and change tail
            if (prev){
                prev->next = cur -> next;
            }
            task* tmp = cur->next;
            cur->next = NULL;
            tail->next = cur;
            tail = cur;
            cur = tmp;
        }
    }
    //if pivot's length is the smallest element in the current list, pivot becomes the head
    if ((*newHead) == NULL){
        (*newHead) = pivot;
    }
    //update newEnd to the current last instruction
    (*newEnd) = tail;
    //return the pivot instruction
    return pivot;
}

//where the real sorting happens
task* quickSortRecur(task* head, task* end){
    //base condition
    if (!head || head == end){
        return head;
    }
    task* newHead = NULL;
    task* newEnd = NULL;

    //partition the list, newHead and newEnd will be updated by the partition function
    task* pivot = partition(head, end, &newHead, &newEnd);

    //if the pivot is the smallest element there is no need to recur for the left part
    if (newHead != pivot){
        //set the instruction before the pivot instruction as NULL
        task* tmp = newHead;
        while (tmp->next != pivot){
            tmp = tmp->next;
        }
        tmp->next = NULL;

        //recur for the list before the pivot
        newHead = quickSortRecur(newHead, tmp);

        //change next of last node to the left half of the pivot
        tmp = getTail(newHead);
        tmp->next = pivot;
    }
    //recur for the list after the pivot element
    pivot->next = quickSortRecur(pivot->next, newEnd);

    return newHead;
}

//the main function for quicksort. This is a wrapper over recursive function quickSortRecur()
void quickSort(task** headRef){
    (*headRef) = quickSortRecur(*headRef, getTail(*headRef));
    return;
}

//quicksort for every list of instraction inside a task's queue
void quickSortOverQueue(queueTask* queue){
    quickSort(&queue->firstTask);
}

/*QUICK SORT FINE -----------------------------------------------------------*/

void* schedule(void* schedInfo){

    schedInfo = newScheduler(((schedulerInfo_t*)schedInfo)->core, ((schedulerInfo_t*)schedInfo)->outputFile, ((schedulerInfo_t*)schedInfo)->taskList, ((schedulerInfo_t*)schedInfo)->schedulerType);

    int Core = ((schedulerInfo_t*) schedInfo)->core;
    char *OutPut = ((schedulerInfo_t*) schedInfo)->outputFile;
    queueTask *TaskList = ((schedulerInfo_t*) schedInfo)->taskList;
    bool SchedulerType = ((schedulerInfo_t*) schedInfo)->schedulerType;

    int clock = 1;

    bool done = false;

    while (((schedulerInfo_t*)schedInfo) -> taskList -> firstTask -> arrival_time > clock){
        clock++;
    }

    //SPN
    /*
    1. QuickSort on taskList instruction
    2. schedule like FCFS
    */
    if ((((schedulerInfo_t*)schedInfo)->schedulerType) == true) {
        queueTask* blockedTask = newQueueForTask();
        //queueTask* readyTask = newQueueForTask();

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
                removeTaskFromQueue(((schedulerInfo_t*)schedInfo)->taskList);
            }
            pthread_mutex_unlock(&mutexOne);

            if (!isEmpty(blockedTask)){
                if ((*blockedTask).firstTask->blockTime <= clock){
                    (*blockedTask).firstTask -> processState = 1;
                    printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*blockedTask).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                    //faccio diventare non bloccante l'istruzione che mi ha bloccato
                    (*blockedTask).firstTask->instr_list->headInstruction->typeFlag = 0; //non bloccante
                    (*blockedTask).firstTask->instr_list->headInstruction->length = (*blockedTask).firstTask->instr_list->headInstruction->executionTime; //la faccio eseguire per la lunghezza e non per il tempo che è stata bloccata
                    //la metto nei ready
                    insertTaskInQueue((*blockedTask).firstTask, readyTask);
                    //tolgo dai blocked
                    removeTaskFromQueue(blockedTask);
                }
            }
           
            if (!isEmpty(readyTask)){
                instruction *currentInstruction = (*readyTask).firstTask->instr_list->headInstruction;
                
                while (currentInstruction != NULL){
                    //istruzione non bloccante
                    if (currentInstruction->typeFlag == 0){
                        (*readyTask).firstTask->processState = 2; //running
                        printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*readyTask).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);

                        int durata = currentInstruction->length + clock;
                        while (clock != durata){
                            clock++;
                        }

                        //se l'istruzione è l'ultima del task
                        if (currentInstruction->next == NULL){
                            (*readyTask).firstTask->processState = 4;
                            printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*readyTask).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);                            
                            removeInstructionFromTask((*readyTask).firstTask);
                            removeTaskFromQueue(readyTask);
                            break;
                        }
                        removeInstructionFromTask((*readyTask).firstTask);
                        currentInstruction = (*currentInstruction).next;
                    }
                    //istruzione bloccante
                    else {
                        //mando in running e poi in blocked
                        (*readyTask).firstTask->processState = 2; //running
                        printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*readyTask).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                        //blocked
                        (*readyTask).firstTask->processState = 3;
                        printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*readyTask).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);

                        (*readyTask).firstTask->blockTime = clock + currentInstruction->length;

                        insertTaskInQueue((*readyTask).firstTask, blockedTask);
                        break;
                    }
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

        queueTask* readyTaskRR = newQueueForTask();
        queueTask* blockedTaskRR = newQueueForTask();

        while (!done) {

            while (((schedulerInfo_t*)schedInfo) -> taskList -> firstTask -> arrival_time > clock){
                clock++;
            }
            
            pthread_mutex_lock(&mutexOne);
            if (!isEmpty(((schedulerInfo_t*)schedInfo)->taskList)) {
                //loggo l' arrivo a new
                printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, ((schedulerInfo_t*)schedInfo)->taskList->firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                //cambio lo stato a ready e loggo il cambiamento
                ((schedulerInfo_t*)schedInfo)->taskList->firstTask->processState = 1;                
                //lo metto nei ready
                insertTaskInQueue(((schedulerInfo_t*)schedInfo)->taskList->firstTask, readyTaskRR);
                //loggo il cambiamento
                printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, ((schedulerInfo_t*)schedInfo)->taskList->firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);

                removeTaskFromQueue(((schedulerInfo_t*)schedInfo)->taskList);
            }
            pthread_mutex_unlock(&mutexOne);

            if (!isEmpty(blockedTaskRR)) {

                if ((*blockedTaskRR).firstTask->blockTime <= clock){
                    (*blockedTaskRR).firstTask -> processState = 1; //ready
                    printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*blockedTaskRR).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                    //faccio diventare non bloccante l'istruzione che mi ha bloccato
                    (*blockedTaskRR).firstTask->instr_list->headInstruction->typeFlag = 0; //non bloccante
                    (*blockedTaskRR).firstTask->instr_list->headInstruction->length = (*blockedTaskRR).firstTask->instr_list->headInstruction->executionTime; //la faccio eseguire per la lunghezza e non per il tempo che è stata bloccata
                    //la metto nei ready
                    insertTaskInQueue((*blockedTaskRR).firstTask, readyTaskRR);
                    //tolgo dai blocked
                    removeTaskFromQueue(blockedTaskRR);
                }
            }

            if (!isEmpty(readyTaskRR)){
                
                //istruzione non bloccante
                if (readyTaskRR->firstTask->instr_list->headInstruction->typeFlag == 0){
                    //mando in running e loggo
                    readyTaskRR->firstTask->processState = 2;
                    printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, readyTaskRR->firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                    
                    //se l'istruzione sta tutta dentro il quanto di tempo e non è l'ultima del task --> NON vado in exit
                    if (readyTaskRR->firstTask->instr_list->headInstruction->length <= timeQuantum && readyTaskRR->firstTask->instr_list->headInstruction->next != NULL){
                        //faccio aumentare il clock della lunghezza dell' istruzione
                        int endTime = clock + readyTaskRR->firstTask->instr_list->headInstruction->length;
                        while (clock != endTime){
                            clock++;
                        }
                        //tolgo l'istruzione dal task
                        removeInstructionFromTask(readyTaskRR->firstTask);
                    }
                    //se l'istruzione sta tutta dentro il quanto di tempo ed è l'ultima del task --> VADO in exit
                    else if (readyTaskRR->firstTask->instr_list->headInstruction->length <= timeQuantum && readyTaskRR->firstTask->instr_list->headInstruction->next == NULL){
                        //faccio aumentare il clock della lunghezza dell' istruzione
                        int endTime = clock + readyTaskRR->firstTask->instr_list->headInstruction->length;
                        while (clock != endTime){
                            clock++;
                        }
                        //mando in exit e loggo
                        readyTaskRR->firstTask->processState = 4;
                        printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, readyTaskRR->firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                        //tolgo l'istruzione dal task e il task dalla coda dei ready
                        removeInstructionFromTask(readyTaskRR->firstTask);
                        removeTaskFromQueue(readyTaskRR);
                    }
                    //se l'istruzione non sta tutta dentro il quanto
                    else if (readyTaskRR->firstTask->instr_list->headInstruction->length > timeQuantum){
                        //accorcio la lunghezza dell'istruzione del quanto
                        readyTaskRR->firstTask->instr_list->headInstruction->length = readyTaskRR->firstTask->instr_list->headInstruction->length - timeQuantum;
                        //la metto nei blocked
                        insertTaskInQueue((*readyTaskRR).firstTask, readyTaskRR);
                        //cambio lo stato e lo loggo
                        (*readyTaskRR).firstTask->processState = 1;
                        printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, (*readyTaskRR).firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                    }
                }

                //istruzione bloccante
                else{
                    //mando in running e loggo
                    readyTaskRR->firstTask->processState = 2;
                    printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, readyTaskRR->firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                    //mando in blocked e loggo
                    readyTaskRR->firstTask->processState = 3;
                    readyTaskRR->firstTask->blockTime = clock + readyTaskRR->firstTask->instr_list->headInstruction->length;
                    insertTaskInQueue((*readyTaskRR).firstTask, blockedTaskRR);
                    printSchedulerStatus(((schedulerInfo_t*)schedInfo)->outputFile, readyTaskRR->firstTask, ((schedulerInfo_t*)schedInfo)->core, clock);
                }
            }

            clock++;

            pthread_mutex_lock(&mutexOne);
            if (isEmpty(readyTaskRR) && isEmpty(blockedTaskRR) && isEmpty(((schedulerInfo_t*)schedInfo)->taskList)){
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