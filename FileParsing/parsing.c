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

void parseInput(queueTask *Queue, char* inputFile) {

    char lineType;
    int firstValue, secondValue;
    task *task = NULL;

    FILE *file = fopen(inputFile, "r");

    if (file == NULL) {
        printf("Errore in parseInput");
        exit(1);
    }

    while (fscanf(file, "%c, %d, %d\n", &lineType, &firstValue, &secondValue) != EOF) {
        
        //row parsing
        switch(lineType) {
            //task
            case 't':
                task = newTask(firstValue, secondValue);
                insertTaskInQueue(task, Queue);
                break;
            //istruzione
            case 'i':
                //se è la prima istruzione
                if ((*task).programCounter == NULL) {
                    instruction *head;
                    //se l'istruzione è non bloccante
                    if (firstValue == 0) {
                        head = newInstruction(false, secondValue);
                    }
                    else {
                        head = newInstruction(true, secondValue);    
                    }
                    insertInstructionInTask(head, task);
                }
                else {
                    instruction *next;
                    if (firstValue == 0) {
                        next = newInstruction(false, secondValue);
                    }
                    else {
                        next = newInstruction(true, secondValue);
                    }
                    insertInstructionInTask(next, task);
                }
                break;
            default:
                printf("Errore in parseInput2");
        }
    }
    fclose(file);
}