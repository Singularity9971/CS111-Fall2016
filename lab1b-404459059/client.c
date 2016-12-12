#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <mcrypt.h>
#include <termios.h>
#include <fcntl.h>

struct termios saved_attributes;
static int encrypt_flag = 0;

char* key = NULL;
char* IV = NULL;
int sockfd;
int fp = -1;
char * log_file = NULL;
long fsize = -1;

void free_key(){
    if(key != NULL){
        free(key);
    }
}

void reset_input_mode()
{
    if(encrypt_flag)
        free_key();
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

void decrypt(char* ch, int size){
    MCRYPT td_decrypt;
    td_decrypt = mcrypt_module_open("twofish", NULL, "cfb", NULL);
    mcrypt_generic_init(td_decrypt, (void*)key, fsize, (void*)IV);
    mdecrypt_generic(td_decrypt, ch, size);
    mcrypt_generic_deinit (td_decrypt);
    mcrypt_module_close(td_decrypt);
}

void encrypt(char* ch, int size){
    MCRYPT td_encrypt;
    td_encrypt = mcrypt_module_open("twofish", NULL, "cfb", NULL);
    mcrypt_generic_init(td_encrypt, (void*)key, fsize, (void*)IV);
    mcrypt_generic(td_encrypt, ch, size);
    mcrypt_generic_deinit (td_encrypt);
    mcrypt_module_close(td_encrypt);
}

void* read_socket(){
    int n;
    char buffer[1024];
    while((n = read(sockfd, &buffer, 1023))){
        if(fp != -1){
            char buf1[] = "RECEIVED ";
            char buf2[6];
            int size = sprintf(buf2, "%d", n);
            char buf3[] = " bytes: ";
            write(fp, buf1, 9);
            write(fp, buf2, size);
            write(fp, buf3, 8);
            write(fp, buffer, n);
            write(fp, "\n",1);
        }
        if(encrypt_flag)
            decrypt(buffer, n);
        write(STDOUT_FILENO, buffer, n);
    }
    shutdown(sockfd, 2);
    exit(1);
}

void checkargs(int argc, char ** argv, int* port){
    int c;
    while (1)
    {
        static struct option long_options[] =
        {
            {"log", required_argument, 0, 'l'},
            {"encrypt", no_argument, &encrypt_flag, 1},
            {"port", required_argument, 0, 'p'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        
        c = getopt_long (argc, argv, "l:p:",
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
                
            case 'l':
                log_file = optarg;
                break;
                
            case '?':
                break;
                
            default:
                abort ();
        }
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


int main(int argc, char ** argv) {
    int port = -1;
    checkargs(argc, argv, &port);
    if(port == -1){
        fprintf(stderr, "Port not passed in, assigning magic number 7777\n");
        port = 7777;
    }
    if(log_file != NULL){
        fp = creat(log_file, 0666);
        if(fp < 0){
            perror("Unable to open file");
            exit(1);
        }
    }
    if(encrypt_flag){
        fetch_key();
        set_up_encryption();
    }
    screw_terminal();   //Put it into non echo and non canonical
    struct sockaddr_in serv_addr;
    struct hostent *server;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("ERROR opening socket");
        exit(1);
    }
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((void *)&serv_addr.sin_addr.s_addr,
          (void *)server->h_addr,
          server->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
        perror("ERROR connecting");
        exit(1);
    }
    char ch;
    char crlf[] = {0x0D, 0x0A};
    pthread_t thread1;
    pthread_create(&thread1, 0, &read_socket, 0);
    char log_buffer[50];
    while(read(STDIN_FILENO, &ch, 1)){
        char lf = 0x0A;
        if(ch == 0x0D || ch == 0x0A){
            write(STDOUT_FILENO, crlf, 2);
            if(encrypt_flag)
                encrypt(&lf, 1);
            if(fp != -1){
                int log_size = sprintf(log_buffer, "SENT %d bytes: %c\n", 1, lf);
                write(fp, log_buffer, log_size);
            }
            write(sockfd, &lf, 1);
        }
        else if(ch == '\004'){
            pthread_cancel(thread1);
            shutdown(sockfd, 2);
            exit(0);
        }
        else{
            write(STDOUT_FILENO, &ch, 1);
            if(encrypt_flag)
                encrypt(&ch, 1);
            if(fp != -1){
                int log_size = sprintf(log_buffer, "SENT %d bytes: %c\n", 1, ch);
                write(fp, log_buffer, log_size);
            }

            write(sockfd, &ch, 1);
        }
    }
}
