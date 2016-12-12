#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

static int num_threads = 1;
static int iterations = 1;
static long long counter;
static struct timespec start_time;
static struct timespec end_time;
static int opt_yield = 0;
static char* sync_option = NULL;
static pthread_mutex_t lock;
static int spin_lock = 0;
static void (*func)(long long*,long long) = NULL;


void error(char* message){
    fputs(message,stderr);
    fputs("\n",stderr);
    exit(1);
}

void checkargs(int argc, char ** argv){
    int c;
    while (1)
    {
        static struct option long_options[] =
        {
            {"threads", required_argument, 0, 't'},
            {"iterations", required_argument, 0, 'i'},
            {"yield", no_argument, &opt_yield, 1},
            {"sync", required_argument, 0, 's'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        
        c = getopt_long (argc, argv, "tis",
                         long_options, &option_index);
        
        if (c == -1)
            break;
        
        switch (c)
        {
            case 0:
                break;
                
            case 'i':
                iterations = atoi(optarg);
                break;
                
            case 't':
                num_threads = atoi(optarg);
                break;
                
            case 's':
                sync_option = optarg;
                break;

            case '?':
                break;
                
            default:
                abort ();
        }
    }
    
}

void set_time(struct timespec* temp){
    if(clock_gettime(CLOCK_MONOTONIC, temp)!=0)
        error("Clock get time failed");
}

void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if(opt_yield)
        sched_yield();
    *pointer = sum;
}

void add_mutex(long long *pointer, long long value) {
    pthread_mutex_lock(&lock);
    add(pointer, value);
    pthread_mutex_unlock(&lock);
}

void add_atomic(long long *pointer, long long value) {
    long long old_value;
    long long new_value;
    do{
        old_value = *pointer;
        new_value = old_value + value;
        if(opt_yield)
            sched_yield();
    }
    while(__sync_val_compare_and_swap(pointer, old_value, new_value) != old_value);
}

void add_spin(long long *pointer, long long value) {
    while(__sync_lock_test_and_set(&spin_lock, 1));
    add(pointer, value);
    __sync_lock_release(&spin_lock);
}

void* thread_function (){
    int i;
    for(i = 0; i < iterations; i++)
        func(&counter, 1);
    for(i = 0; i < iterations; i++)
        func(&counter, -1);

    return NULL;
}

void write_out(char* type){
    int operations = num_threads * iterations * 2;
    long long run_time = (end_time.tv_sec - start_time.tv_sec) * 1000000000L;
    run_time += (end_time.tv_nsec - start_time.tv_nsec);
    printf("%s,%d,%d,%d,%lld,%lld,%lld\n",type,num_threads,iterations,
           operations,run_time,run_time/operations,counter);
    
}

void start_process(){
    pthread_t threads[num_threads];
    set_time(&start_time);
    int i;
    for(i = 0; i < num_threads; ++i)
        pthread_create(&threads[i], NULL, thread_function, NULL);
    for(i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);
    set_time(&end_time);

}

int main(int argc, char ** argv) {
    checkargs(argc, argv);
    counter = 0;
    if(sync_option == NULL){
        func = &add;
        start_process();
        if(opt_yield)
            write_out("add-yield-none");
        else
            write_out("add-none");
        exit(0);
    }
    if(*sync_option == 'm'){
        pthread_mutex_init(&lock, NULL);
        func = &add_mutex;
        start_process();
        pthread_mutex_destroy(&lock);
        if(opt_yield)
            write_out("add-yield-m");
        else
            write_out("add-m");
        exit(0);
    }
    if(*sync_option == 's'){
        func = &add_spin;
        start_process();
        if(opt_yield)
            write_out("add-yield-s");
        else
            write_out("add-s");
        exit(0);
    }
    if(*sync_option == 'c'){
        func = &add_atomic;
        start_process();
        if(opt_yield)
            write_out("add-yield-c");
        else
            write_out("add-c");
        exit(0);
    }
    
}
