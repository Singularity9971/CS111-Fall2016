#include <stdio.h>
#include <mraa.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#define ADC_PIN 0

const int b_thermistor = 4277;
const int resistance = 100;
static const char format[] = "%H:%M:%S";
int celsius = 0;
int delay = 3;
int port = 16000;
const char server_name[] = "lever.cs.ucla.edu";
int sockfd = 0;
int should_output = 1;
int log_file;
pthread_mutex_t lock;
mraa_aio_context adc_pin;

void write_to_log(const char* to_write){
    pthread_mutex_lock(&lock);
    size_t num_bytes = strlen(to_write);
    write(log_file, to_write, num_bytes);
    pthread_mutex_unlock(&lock);
}

void connect_to_server(int port_num){
    struct sockaddr_in serv_addr;
    struct hostent *server;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("ERROR opening socket");
        exit(1);
    }
    server = gethostbyname(server_name);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((void *)&serv_addr.sin_addr.s_addr,
           (void *)server->h_addr,
           server->h_length);
    serv_addr.sin_port = htons(port_num);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
        perror("ERROR connecting");
        exit(1);
    }
}

void init(){
    mraa_init();
    adc_pin = mraa_aio_init(ADC_PIN);
    pthread_mutex_init(&lock, NULL);
    log_file = creat("logb.txt", 0666);
}

float get_temperature(char scale){
    float adc_value, res, temperature;
    adc_value = mraa_aio_read(adc_pin);
    res = 1023.0/((float)adc_value)-1.0;
    res = 100000.0*res;
    temperature = 1.0/(log(res/100000.0)/b_thermistor+1/298.15)-273.15;
    if(scale == 'F')
        temperature = (1.8*temperature) + 32;
    return temperature;
}

void get_port_number(){
    const char message[] = "Port request 404459059";
    connect_to_server(port);
    write(sockfd, message, sizeof(message));
    size_t count = 0;
    port = 0;
    while(!count){
        count = read(sockfd, &port, 8);
    }
    shutdown(sockfd, SHUT_RDWR);
    connect_to_server(port);
}

void parse_command(char* command, size_t len){
    int invalid = 0;
    char* start;
    char buffer[256] = {0};
    if(strcmp(command, "START") == 0)
        should_output = 1;
    else if(strcmp(command, "OFF") == 0){
        sprintf(buffer, "%s\n", command);
        write_to_log(buffer);
	write(0, buffer, strlen(buffer));
        exit(0);
    }
    else if(strcmp(command, "STOP") == 0)
        should_output = 0;
    else if(strcmp(command, "SCALE=C") == 0)
        celsius = 1;
    else if(strcmp(command, "SCALE=F") == 0)
        celsius = 0;
    else if((start = strstr(command, "FREQ=")) != NULL){
        char* num_start = start + 5;
        int freq_num = atoi(num_start);
        if(freq_num >= 1 && freq_num <= 3600)
            delay = freq_num;
        else
            invalid = 1;
    }
    else
        invalid = 1;
    
    if(invalid){
        sprintf(buffer, "%s I\n", command);
        write_to_log(buffer);
    }
    else{
        sprintf(buffer, "%s\n", command);
        write_to_log(buffer);
    }
    write(0, buffer, strlen(buffer));
}

void* read_server(){
    size_t n;
    char buffer[256] = {0};
    while(1){
        n = read(sockfd, buffer, 256);
        parse_command(buffer, n);
        memset(buffer, 0, 256);
    }
}

void log_temperature(){
    float temperature;
    setenv("TZ", "PST8PST", 1);
    tzset();
    char scale;
    time_t timestamp;
    struct tm* time_struct;
    while(1){
        char send_server_buf[64] = {0};
        char send_log_buf[64] = {0};
        char output[10] = {0};
        if(celsius)
            scale = 'C';
        else
            scale = 'F';
        temperature = get_temperature(scale);
        time(&timestamp);
        time_struct = localtime(&timestamp);
        strftime(output, sizeof(output), format, time_struct);
        if(!should_output)
            continue;
        sprintf(send_server_buf, "404459059 TEMP=%0.1f\n", temperature);
        sprintf(send_log_buf, "%s %0.1f %c\n", output, temperature, scale);
        write(sockfd, send_server_buf, strlen(send_server_buf));
	write(0, send_log_buf, strlen(send_log_buf));
        write_to_log(send_log_buf);
        sleep(delay);
    }
}

int main(int argc, const char * argv[]) {
    init();
    get_port_number();
    pthread_t thread1;
    pthread_create(&thread1, 0, &read_server, 0);
    log_temperature();
    return 0;
}
