#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <mcrypt.h>

static int encrypt_flag = 0;
char* key = NULL;
char* IV = NULL;
pid_t cpid;
int newsockfd;
long fsize = -1;

void sigpipe_handler(int sig){
    shutdown(newsockfd, 2);
    kill(cpid, SIGKILL);
    exit(2);
}

struct argument{
    int shell_pipe;
};

void checkargs(int argc, char ** argv, int* port){
    int c;
    while (1)
    {
        static struct option long_options[] =
        {
            {"encrypt", no_argument, &encrypt_flag, 1},
            {"port", required_argument, 0, 'p'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        
        c = getopt_long (argc, argv, "p:",
                         long_options, &option_index);
        
        if (c == -1)
            break;
        
        switch (c)
        {
            case 0:
                break;
            
            case 'p':
                *port = atoi(optarg);
                break;
                
            case '?':
                break;
                
            default:
                abort ();
        }
    }
    
}

void decrypt(char* ch, int size){
    MCRYPT td_decrypt;
    td_decrypt = mcrypt_module_open("twofish", NULL, "cfb", NULL);
    mcrypt_generic_init(td_decrypt, (void*)key, fsize, (void*)IV);
    mdecrypt_generic(td_decrypt, (void*)ch, size);
    mcrypt_generic_deinit (td_decrypt);
    mcrypt_module_close(td_decrypt);
}

void encrypt(char* ch, int size){
    MCRYPT td_encrypt;
    td_encrypt = mcrypt_module_open("twofish", NULL, "cfb", NULL);
    mcrypt_generic_init(td_encrypt, (void*)key, fsize, (void*)IV);
    mcrypt_generic(td_encrypt, (void*)ch, size);
    mcrypt_generic_deinit (td_encrypt);
    mcrypt_module_close(td_encrypt);
}

void* read_shell(void* arguments){
    struct argument* args = arguments;
    char buffer[1024];
    int n;
    while((n = read(args->shell_pipe, buffer, 1024))){
        if(encrypt_flag)
            encrypt(buffer, n);
        write(STDOUT_FILENO, buffer, n);
    }
    shutdown(newsockfd, 2);
    kill(cpid, SIGKILL);
    exit(2);
}

void fetch_key(){
    FILE* fp = fopen("my.key", "r");
    if(fp == NULL){
        perror("Can't open my.key");
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    rewind(fp);
    key = malloc(fsize+1);
    fread(key, fsize, 1, fp);
    key[fsize] = 0;
    fclose(fp);
}

void free_key(){
    if(key != NULL){
        free(key);
    }
}

void set_up_encryption(){
    MCRYPT td;
    td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
    IV = malloc(mcrypt_enc_get_iv_size(td));
    int i;
    for (i=0; i< mcrypt_enc_get_iv_size(td); i++) {
        *(IV + i) = i;
    }
    mcrypt_generic_deinit (td);
    mcrypt_module_close(td);
}

int main(int argc, char ** argv) {
    int port = -1;
    checkargs(argc, argv, &port);
    if(port == -1){
        fprintf(stderr, "Port not passed in, assigning magic number 7777\n");
        port = 7777;
    }
    if(encrypt_flag){
        fetch_key();
        set_up_encryption();
        atexit(free_key);
    }
    int sockfd;
    unsigned int clilen;
    struct sockaddr_in serv_addr, cli_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("Socket creation failed");
        exit(1);
    }
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0){
        perror("ERROR on binding");
        exit(1);
    }
    printf("%i", htons(serv_addr.sin_port));
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0){
        perror("ERROR on accept");
        exit(1);
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
        dup2(from_child_pipe[1], STDERR_FILENO);
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
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        if(dup(newsockfd) != 0 || dup(newsockfd) != 1 || dup(newsockfd) != 2) {
            perror("Error redirecting stdin/stdout/stderr of server");
            exit(1);
        }
        close(to_child_pipe[0]);
        close(from_child_pipe[1]);
        char ch;
        char lf = 0x0A;
        struct argument args;
        args.shell_pipe = from_child_pipe[0];
        pthread_t thread1;
        pthread_create(&thread1, 0, &read_shell, (void *)&args);
        while(read(STDIN_FILENO, &ch, 1)){
            if(encrypt_flag)
                decrypt(&ch, 1);
            if(ch == 0x0D || ch == 0x0A){
                write(to_child_pipe[1], &lf, 1);
            }
            else{
                write(to_child_pipe[1], &ch, 1);
            }
        }
        shutdown(newsockfd, 2);
        pthread_cancel(thread1);
        close(to_child_pipe[1]);
        close(from_child_pipe[0]);
        kill(cpid, SIGKILL);
        exit(1);
        
    }
}
