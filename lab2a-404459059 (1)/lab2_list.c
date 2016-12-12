#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "SortedList.h"
#include <string.h>

static int iterations = 1;
static int num_threads = 1;
static char* yield_opt = NULL;
static char* sync_option = NULL;
int opt_yield = 0;
SortedListElement_t* head = NULL;
SortedListElement_t* elements = NULL;
static const char* alpha_num = "ABCDEFGHIJKLMNPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz123456789";
static struct timespec start_time;
static struct timespec end_time;
static char* test_name = NULL;
static int mutex_locking = 0;
static int spin_locking = 0;
static pthread_mutex_t lock;
static int spin_lock = 0;

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
            {"yield", required_argument, 0, 'y'},
            {"sync", required_argument, 0, 's'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        
        c = getopt_long (argc, argv, "tisy",
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
                
            case 'y':
                opt_yield = 1;
                yield_opt = optarg;
                break;
                
            case '?':
                break;
                
            default:
                abort ();
        }
    }
}

char* gen_key(){
    unsigned long len = strlen(alpha_num);
    int key_size = rand() % len;
    if(key_size == 0)
        key_size = 1;
    char* key = malloc(key_size);
    int i;
    for(i = 0; i < key_size; i++){
        key[i] = alpha_num[rand() % len];
    }
    return key;
}

void initialize_list(){
    srand((unsigned int)time(NULL));
    head = malloc(sizeof(SortedListElement_t));
    head->key = NULL;
    head->prev = NULL;
    head->next = NULL;
    int num_elements = num_threads * iterations;
    elements = malloc(sizeof(SortedListElement_t)*num_elements);
    int i;
    for(i = 0; i < num_elements; i++){
        elements[i].next = NULL;
        elements[i].prev = NULL;
        elements[i].key = gen_key();
    }
}

void set_time(struct timespec* temp){
    if(clock_gettime(CLOCK_MONOTONIC, temp)!=0)
        error("Clock get time failed");
}

void lookup_delete(SortedListElement_t* node, int index){
    node = SortedList_lookup(head, elements[index].key);
    if(node == NULL)
        error("Key was inserted but lookup failed");
    if(SortedList_delete(node) != 0)
        error("Node was found but couldn't be deleted");
}

void* thread_function(void* args){
    int end = *((int *)args);
    free(args);
    int start = end - iterations;
    int i;
    for(i = start; i < end; i++){
        if(mutex_locking){
            pthread_mutex_lock(&lock);
            SortedList_insert(head, &elements[i]);
            pthread_mutex_unlock(&lock);
        }
        else if(spin_locking){
            while(__sync_lock_test_and_set(&spin_lock, 1));
            SortedList_insert(head, &elements[i]);
            __sync_lock_release(&spin_lock);
        }
        else
            SortedList_insert(head, &elements[i]);
    }
    if(mutex_locking){
        pthread_mutex_lock(&lock);
        SortedList_length(head);
        pthread_mutex_unlock(&lock);
    }
    else if(spin_locking){
        while(__sync_lock_test_and_set(&spin_lock, 1));
        SortedList_length(head);
        __sync_lock_release(&spin_lock);
    }
    else
        SortedList_length(head);
    SortedListElement_t* node = NULL;
    for(i = start; i < end; i++){
        if(mutex_locking){
            pthread_mutex_lock(&lock);
            lookup_delete(node,i);
            pthread_mutex_unlock(&lock);
        }
        else if(spin_locking){
            while(__sync_lock_test_and_set(&spin_lock, 1));
            lookup_delete(node,i);
            __sync_lock_release(&spin_lock);
        }
        else{
            lookup_delete(node,i);
        }
    }
    return NULL;
}

void start_process(){
    pthread_t threads[num_threads];
    set_time(&start_time);
    int i;
    for(i = 0; i < num_threads; ++i){
        int* end = malloc(sizeof(*end));
        *end = (i+1)*iterations;
        pthread_create(&threads[i], NULL, thread_function, (void*)end);
    }
    for(i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);
    if(mutex_locking)
        pthread_mutex_destroy(&lock);
    set_time(&end_time);
}

void parse_arguments(){
    char* yield_name = NULL;
    char* sync_name = NULL;
    if(yield_opt == NULL){
        yield_name = "none";
    }
    else{
        yield_name = malloc(4);
        memset(yield_name, 0, 4);
	if(strchr(yield_opt, 'i') != NULL){
	  opt_yield |= INSERT_YIELD;
	  strcat(yield_name, "i");
        }
        if(strchr(yield_opt, 'd') != NULL){
	  opt_yield |= DELETE_YIELD;
	  strcat(yield_name, "d");
        }
        if(strchr(yield_opt, 'l') != NULL){
	  opt_yield |= LOOKUP_YIELD;
	  strcat(yield_name, "l");
        }
    }
    if(sync_option == NULL){
        sync_name = "none";
    }
    else{
        sync_name = malloc(1);
        memset(sync_name, 0, 1);
        int i;
        for(i = 0; i < strlen(sync_option); i++){
            switch (sync_option[i]) {
                case 'm':
                    strcat(sync_name, "m");
                    mutex_locking = 1;
                    pthread_mutex_init(&lock, NULL);
                    break;
                case 's':
                    strcat(sync_name, "s");
                    spin_locking = 1;
                    break;
                default:
                    break;
            }
        }
    }
    test_name = malloc(20);
    sprintf(test_name, "%s-%s-%s", "list", yield_name, sync_name);
    if(sync_option != NULL)
        free(sync_name);
    if(yield_opt != NULL)
        free(yield_name);
}

void write_out(char* type){
    int operations = num_threads * iterations * 3;
    long long run_time = (end_time.tv_sec - start_time.tv_sec) * 1000000000L;
    run_time += (end_time.tv_nsec - start_time.tv_nsec);
    printf("%s,%d,%d,%d,%d,%lld,%lld\n",type,num_threads,iterations,
           1,operations,run_time,run_time/operations);
}

void destroy(){
    free(test_name);
    free(head);
    int i;
    for(i = 0; i < num_threads*iterations; i++)
        free((void*)elements[i].key);
    free(elements);
}

int main(int argc, char ** argv){
    checkargs(argc, argv);
    initialize_list();
    parse_arguments();
    start_process();
    if(SortedList_length(head) != 0){
        error("Length of list is not 0 after completion");
    }
    write_out(test_name);
    destroy();
    exit(0);
}

