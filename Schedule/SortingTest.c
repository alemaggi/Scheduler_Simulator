#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

//#include "list.c"
#include "parser.c"

//USIAMO QUICKSORT PERCHE PER ORDINARE TUTTE LE ISTRUZIONI CI METTE MENO DI 3 SECONDI

//return the last element of a list (posso sostituirlo con un attributo nella struttura)
instruction* getTail(instruction* current){
    while (current != NULL && (*current).next != NULL){
        current = current->next;
    }
    return current;
}


//partitions the list taking the last element as the pivot
instruction* partition(instruction* head, instruction* end, instruction** newHead, instruction** newEnd){
    instruction* pivot = end;
    instruction* prev = NULL;
    instruction* cur = head; 
    instruction* tail = pivot;

    // During partition, both the head and end of the list might change 
    // which is updated in the newHead and newEnd variables
    while (cur != pivot){
        if (cur->length < pivot->length){
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
            instruction* tmp = cur->next;
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
instruction* quickSortRecur(instruction* head, instruction* end){
    //base condition
    if (!head || head == end){
        return head;
    }
    instruction* newHead = NULL;
    instruction* newEnd = NULL;

    //partition the list, newHead and newEnd will be updated by the partition function
    instruction* pivot = partition(head, end, &newHead, &newEnd);

    //if the pivot is the smallest element there is no need to recur for the left part
    if (newHead != pivot){
        //set the instruction before the pivot instruction as NULL
        instruction* tmp = newHead;
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
void quickSort(instruction** headRef){
    (*headRef) = quickSortRecur(*headRef, getTail(*headRef));
    return;
}

//quicksort for every list of instraction inside a task's queue
void quickSortOverQueue(queueTask* queue){
    for (task* currentTask = queue->firstTask; currentTask != NULL; currentTask = currentTask->next){
        quickSort(&currentTask->instr_list->headInstruction);
    }
}

int main(int argc, char const *argv[]){
    
    queueTask *Queue = newQueueForTask();
    
    FILE *fopen( const char * filename, const char * mode );

    char* input = "01_tasks.csv";

    parseInput(Queue, input);

    quickSortOverQueue(Queue);

    printAllTask(Queue);

    return 0;
}