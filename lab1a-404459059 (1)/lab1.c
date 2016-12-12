//
//  main.c
//  Project1
//
//  Created by Avirudh Theraja on 10/3/16.
//  Copyright © 2016 Avirudh Theraja. All rights reserved.
//
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

struct termios saved_attributes;
static int shell_flag = 0;
pid_t cpid;

void print_child_status(){
    int status;
    waitpid(cpid, &status, 0);
    if(WIFEXITED(status))
    {
        printf("Shell exited with status: %d\n", WEXITSTATUS(status));
    }
    else if(WIFSIGNALED(status))
    {
        printf("Shell exited due to a signal with status: %d\n", WTERMSIG(status));
    }
    else {
        printf("Shell exited");
    }
}

void sigpipe_handler(int sig){
    exit(1);
}

struct argument{
    int shell_pipe;
};

void reset_input_mode()
{
    if(shell_flag){
        print_child_status();
    }
    tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

void screw_terminal(){
    struct termios tattr;
    
    if (!isatty (STDIN_FILENO))
    {
        fprintf (stderr, "Not a terminal.\n");
        exit (EXIT_FAILURE);
    }
    
    tcgetattr (STDIN_FILENO, &saved_attributes);
    atexit(reset_input_mode);
    
    tcgetattr (STDIN_FILENO, &tattr);
    tattr.c_lflag &= ~(ICANON|ECHO);
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}

void checkargs(int argc, char ** argv){
    int c;
    while (1)
    {
        static struct option long_options[] =
        {
            {"shell", no_argument, &shell_flag, 1},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        
        c = getopt_long (argc, argv, "",
                         long_options, &option_index);
        
        if (c == -1)
            break;
        
        switch (c)
        {
            case 0:
                break;
                
            case '?':
                break;
                
            default:
                abort ();
        }
    }

}

void* read_shell(void* arguments){
    struct argument* args = arguments;
    char buffer;
    while(read(args->shell_pipe, &buffer, 1)){
        if(buffer == '\004'){
            exit(1);
        }
        write(STDOUT_FILENO, &buffer, 1);
    }
    exit(1);
}

int main(int argc, char ** argv) {
    checkargs(argc, argv);  //Set shell flag to 1 if --shell
    screw_terminal();   //Put it into non echo and non canonical
    if(!shell_flag){
        //Part 1 of Project
        char ch;
        char crlf[] = {0x0D, 0x0A};
        while(read(STDIN_FILENO, &ch, 1)){
            if(ch == 0x0D || ch == 0x0A){
                write(STDOUT_FILENO, crlf, 2);
            }
            else if(ch == '\004'){
                exit(0);
            }
            else{
                write(STDOUT_FILENO, &ch, 1);
            }
        }
        exit(0);
    }
    int to_child_pipe[2];
    int from_child_pipe[2];
    if(pipe(to_child_pipe) == -1){
        fprintf(stderr, "Pipe failed");
        exit(1);
    }
    if(pipe(from_child_pipe) == -1){
        fprintf(stderr, "Pipe failed");
        exit(1);
    }
    signal(SIGPIPE, sigpipe_handler);
    cpid = fork();
    if(cpid == -1){
        perror("Fork failed");
        exit(1);
    }
    else if(cpid == 0){
        close(to_child_pipe[1]);
        close(from_child_pipe[0]);
        dup2(to_child_pipe[0], STDIN_FILENO);
        dup2(from_child_pipe[1], STDOUT_FILENO);
        close(to_child_pipe[0]);
        close(from_child_pipe[1]);
        
        char * execvp_argx[2];
        char execvp_file[] = "/bin/bash";
        execvp_argx[0] = execvp_file;
        execvp_argx[1] = 0;
        if(execvp(execvp_file, execvp_argx) == -1){
            printf("Execvp failed");
            exit(1);
        }
    }
    else{
        close(to_child_pipe[0]);
        close(from_child_pipe[1]);
        char ch;
        char crlf[] = {0x0D, 0x0A};
        char lf = 0x0A;
        struct argument args;
        args.shell_pipe = from_child_pipe[0];
        pthread_t thread1;
        pthread_create(&thread1, 0, &read_shell, (void *)&args);
        while(read(STDIN_FILENO, &ch, 1)){
            if(ch == 0x0D || ch == 0x0A){
                write(STDOUT_FILENO, crlf, 2);
                write(to_child_pipe[1], &lf, 1);
            }
            else if(ch == '\003'){
                kill(cpid, SIGINT);
                pthread_cancel(thread1);
            }
            else if(ch == '\004'){
                pthread_cancel(thread1);
                close(to_child_pipe[1]);
                close(from_child_pipe[0]);
                kill(cpid, SIGHUP);
                exit(0);
            }
            else{
                write(STDOUT_FILENO, &ch, 1);
                write(to_child_pipe[1], &ch, 1);
            }
        }
    }
}
