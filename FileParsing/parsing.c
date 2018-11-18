//
//
//  Created by Alessandro Maggi on 30/10/2018.
//  Copyright © 2018 Alessandro Maggi. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "./../DataStructure/list.c"

void parseInput(queueTask *Queue, char *inputFile) {

    char lineType;
    int firstValue, secondValue;
    task *task = NULL;

    FILE *file = fopen(inputFile, "r");

    if (file == NULL) {
        printf("Errore"); //fare qualcosa piu utile di stampare errore
    }

    while (fscanf(inputFile, "%c, %i, %i", &lineType, &firstValue, &secondValue)) {
        switch(lineType) {
            case 't':
                task = newTask(firstValue, secondValue);
                insertTaskInQueue;
                break;
            case 'i':
                //se è la prima istruzione
                if ((*task).programCounter == NULL) {
                    //se l'istruzione è non bloccante
                    if (firstValue == 0) {
                        instruction *head = newInstruction(false, secondValue);
                        insertInstructionInTask(head, task);
                    }
                    else {
                        instruction *head = newInstruction(true, secondValue);
                        insertInstructionInTask(head, task);
                    }
                }
                else {
                    if (firstValue == 0) {
                        instruction *head = newInstruction(false, secondValue);
                        insertInstructionInTask(head, task);
                    }
                    else {
                        instruction *head = newInstruction(true, secondValue);
                        insertInstructionInTask(head, task);
                    }
                }
                break;
            default:
                printf("Errore"); //troavre di meglio
        }
    }
    fclose(inputFile);
}