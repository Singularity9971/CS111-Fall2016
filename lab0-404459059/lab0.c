//
//  lab0.c
//  Project0
//
//  Created by Avirudh Theraja on 9/24/16.
//  Copyright © 2016 Avirudh Theraja. All rights reserved.
//
/**
 Citing sources: getopt_long code inspired from https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
**/
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

typedef void (*sighandler_t)(int);

sighandler_t signal(int signum, sighandler_t handler);

void sig_handler(int sig){
    fprintf(stderr, "Segmentation fault encountered, exiting...\n");
    _exit(3);
}

static int catch_flag = 0;
static int seg_flag = 0;

void redirectInput(char * newfile){
    int ifd = open(newfile, O_RDONLY);
    if (ifd >= 0) {
        close(0);
        dup(ifd);
        close(ifd);
    }
    else{
        perror("Input redirection error: ");
        fprintf(stderr, "A problem occured when opening the input file\n");
        _exit(1);
    }
}

void redirectOutput(char * newfile){
    int ofd = creat(newfile, 0666);
    if (ofd >= 0) {
        close(1);
        dup(ofd);
        close(ofd);
    }
    else{
        perror("Output redirection error: ");
        fprintf(stderr, "A problem occured when creating the output file\n");
        _exit(2);
    }
}

void readAndWrite(){
    char c;
    while(read(STDIN_FILENO, &c, 1) != 0){
        write(STDOUT_FILENO, &c, 1);
    }
}

int
main (int argc, char **argv)
{
    int c;
    char * inputFileName = NULL;
    char * outputFileName = NULL;
    while (1)
    {
        static struct option long_options[] =
        {
            {"segfault", no_argument, &seg_flag, 1},
            {"catch",   no_argument,  &catch_flag, 1},
            {"input",  required_argument, 0, 'i'},
            {"output",    required_argument, 0, 'o'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        
        c = getopt_long (argc, argv, "i:o:",
                         long_options, &option_index);
        
        if (c == -1)
            break;
        
        switch (c)
        {
            case 0:
                break;
                
            case 'o':
                outputFileName = optarg;
                break;
                
            case 'i':
                inputFileName = optarg;
                break;
                
            case '?':
                break;
                
            default:
                abort ();
        }
    }
    
    if(catch_flag){
        signal(SIGSEGV, sig_handler);
    }
    if(seg_flag){
        char * disaster = NULL;
        *disaster = 1;
    }
    if(inputFileName != NULL){
        redirectInput(inputFileName);
    }
    if(outputFileName != NULL){
        redirectOutput(outputFileName);
    }
    readAndWrite();
    _exit(0);
}
