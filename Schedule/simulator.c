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
#include "input.c"
#include "parser.c"
#include "scheduler.c"

int main(int argc, char* argv[])
{   
    pid_t Pid; 

    if(Pid == -1){
        exit(-1);
    }
        
    char* input_file = NULL;
    char* output_pree = NULL;
    char* output_no_pree = NULL;

    queueTask* Queue = newQueueForTask();

    cmdOption(argv, argc, &input_file, &output_pree, &output_no_pree);
    
    Pid = fork();

    parseInput(Queue, input_file); 

    if(Pid == 0){
        schedulate(Queue, output_pree, false); // preemptive
    }
    else{
        calcosaSommaIstruzioniTask(Queue);
        quickSortOverQueue(Queue);
        schedulate(Queue, output_no_pree, true); // nonpreemptive
    }

    if(Pid != 0){
        waitpid(Pid, NULL, 0);
    }
    
    free(Queue);
    
    return 0;
}