#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

const char* program_name;

//stampo l' helper dei comandi da cmd line
void print_help(char* program_name) {
    printf("%s %s %s\n%s\n%s\n%s\n%s\n",
        "Use:", program_name,"[OPTION] ...",
        "-h  | --help                         help message",
        "-i  | --input <FILE>                 input-file simulatpr",
        "-on | --output-no-preemption <FILE>  output-file no-preemptive",
        "-op | --output-preemption <FILE>     output-file preemptive"
    );
    exit(0);
}

void cmdOption(char** argv, int argc, char** input_file, char** output_preemption, char** output_no_preemption) {

    const struct option long_options[] = {
        {"output-preeption", 1, NULL, 1},
        {"outout-no-preemption", 1, NULL, 2},
        {"input", 0, NULL, 3},
        {"help", 0, NULL, 4},
        {NULL, 0, NULL, 0}
    };

    const char* optstring = "o:i:h";

    opterr = 0;

    int getopt_result;

    do{
        getopt_result = getopt_long(argc, argv, optstring, long_options, NULL);

        if(getopt_result == -1)
            break;

        switch(getopt_result){
            case 'o':
                if (strlen(optarg) != 1 || optind == argc || *argv[optind] == '-')
                    print_help(argv[0]);

                if(*optarg == 'p'){
                    *output_preemption = argv[optind];
                }else if(*optarg == 'n'){
                    *output_no_preemption = argv[optind];
                }else{
                    print_help(argv[0]);
                    exit(0);
                }
                optind++;
            break;
            
            case 'i':
                *input_file = optarg;
            break;
            
            case 'h':
                print_help(argv[0]);
            break;
            
            default:
                printf("Errore"); //trovare di meglio
            break;
        }
    }while(getopt_result != -1);

    if(*input_file == NULL || *output_preemption == NULL || *output_no_preemption == NULL){
        printf("Errore"); //trovare di meglio
    }
}