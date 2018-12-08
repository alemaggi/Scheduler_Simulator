
/*DA SOLO FUNZIONA*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "list.c"

void parseInput(queueTask *Queue, char* inputFile) {

    char lineType;
    int firstValue, secondValue;
    task *task = NULL;

    FILE *file = fopen(inputFile, "r");

    if (file == NULL) {
        printf("Errore"); //fare qualcosa piu utile di stampare errore
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
                //se è la prima istruzione <-- serve ??????
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
                printf("Errore"); //troavre di meglio
        }
    }
    fclose(file);
}