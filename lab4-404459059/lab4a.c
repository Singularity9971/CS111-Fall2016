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
int delay = 1;
int log_file;
mraa_aio_context adc_pin;

void write_to_log(const char* to_write){
    size_t num_bytes = strlen(to_write);
    write(log_file, to_write, num_bytes);
}

void init(){
    mraa_init();
    adc_pin = mraa_aio_init(ADC_PIN);
    log_file = creat("loga.txt", 0666);
}

float get_temperature(){
    float adc_value, res, temperature;
    adc_value = mraa_aio_read(adc_pin);
    res = 1023.0/((float)adc_value)-1.0;
    res = 100000.0*res;
    temperature = 1.0/(log(res/100000.0)/b_thermistor+1/298.15)-273.15;
    temperature = (1.8*temperature) + 32;
    return temperature;
}

void log_temperature(){
    float temperature;
    setenv("TZ", "PST8PST", 1);
    tzset();
    time_t timestamp;
    int seconds = 60;
    struct tm* time_struct;
    while(seconds){
        char send_log_buf[64] = {0};
        char output[10] = {0};
        temperature = get_temperature();
        time(&timestamp);
        time_struct = localtime(&timestamp);
        strftime(output, sizeof(output), format, time_struct);
        sprintf(send_log_buf, "%s %0.1f\n", output, temperature);
        write(STDOUT_FILENO, send_log_buf, strlen(send_log_buf));
        write_to_log(send_log_buf);
        sleep(delay);
        seconds--;
    }
}

int main(int argc, const char * argv[]) {
    init();
    log_temperature();
    return 0;
}
