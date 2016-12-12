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
SortedListElement_t* elements = NULL;
static struct timespec start_time;
static struct timespec end_time;
static char* test_name = NULL;
static int mutex_locking = 0;
static int spin_locking = 0;
static long long mutex_wait = 0;
static struct SortedListElement* array = NULL;
static int lists = 1;
static pthread_mutex_t* locks = NULL;
static int* spin_locks = NULL;
static long long* thread_array = NULL;

void error(char* message){
  fputs(message,stderr);
  fputs("\n",stderr);
  exit(1);
}

struct threadArgs{
  int end;
  int index;
};

int hash(char* p){
  return *p % lists;
}

long long get_time_lapse(struct timespec* start_time, struct timespec* end_time){
  long long run_time = (end_time->tv_sec - start_time->tv_sec) * 1000000000L;
  run_time += (end_time->tv_nsec - start_time->tv_nsec);
  return run_time;
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
            {"lists", required_argument, 0, 'l'},
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
                
	  case 'l':
	    lists = atoi(optarg);
	    if(lists < 1)
	      error("Number of lists should be positive");
	    break;
                
	  case '?':
	    break;
                
	  default:
	    abort ();
	  }
    }
}

char* gen_key(){
  char* p = malloc(1);
  *p = rand() % 128;
  return p;
}

void initialize_list(){
  srand((unsigned int)time(NULL));
  array = malloc(sizeof(struct SortedListElement)*lists);
  int i;
  for(i = 0; i < lists; i++){
    array[i].key = NULL;
    array[i].next = NULL;
    array[i].prev = NULL;
  }
  int num_elements = num_threads * iterations;
  elements = malloc(sizeof(SortedListElement_t)*num_elements);
  int j;
  for(j = 0; j < num_elements; j++){
    elements[j].next = NULL;
    elements[j].prev = NULL;
    elements[j].key = gen_key();
  }
  thread_array = calloc(num_threads, sizeof(long long));
}

void set_time(struct timespec* temp){
  if(clock_gettime(CLOCK_MONOTONIC, temp)!=0)
    error("Clock get time failed");
}

void lookup_delete(SortedListElement_t* node, int bucket, int index){
  node = SortedList_lookup(&array[bucket], elements[index].key);
  if(node == NULL)
    error("Key was inserted but lookup failed");
  if(SortedList_delete(node) != 0)
    error("Node was found but couldn't be deleted");
}

void* thread_function(void* args){
  int end = ((struct threadArgs*)args)->end;
  int index = ((struct threadArgs*)args)->index;
  free(args);
  struct timespec threadStart_time;
  struct timespec threadEnd_time;
  int start = end - iterations;
  int i;
  int bucket;
  for(i = start; i < end; i++){
    bucket = hash((char*)elements[i].key);
    if(mutex_locking){
      set_time(&threadStart_time);
      pthread_mutex_lock(&locks[bucket]);
      set_time(&threadEnd_time);
      thread_array[index] += get_time_lapse(&threadStart_time, &threadEnd_time);
      SortedList_insert(&array[bucket], &elements[i]);
      pthread_mutex_unlock(&locks[bucket]);
    }
    else if(spin_locking){
      while(__sync_lock_test_and_set(&spin_locks[bucket], 1));
      SortedList_insert(&array[bucket], &elements[i]);
      __sync_lock_release(&spin_locks[bucket]);
    }
    else
      SortedList_insert(&array[bucket], &elements[i]);
  }
  if(mutex_locking){
    set_time(&threadStart_time);
    int j;
    for(j = 0; j < lists; j++)
      pthread_mutex_lock(&locks[j]);
    set_time(&threadEnd_time);
    thread_array[index] += get_time_lapse(&threadStart_time, &threadEnd_time);
    int length = 0;
    for(j = 0; j < lists; j++)
      length += SortedList_length(&array[j]);
    for(j = 0; j < lists; j++)
      pthread_mutex_unlock(&locks[j]);

  }
  else if(spin_locking){
    int j;
    for(j = 0; j < lists; j++)
      while(__sync_lock_test_and_set(&spin_locks[j], 1));
    int length = 0;
    for(j = 0; j < lists; j++)
      length += SortedList_length(&array[j]);
    for(j = 0; j < lists; j++)
      __sync_lock_release(&spin_locks[j]);
  }
  else{
    int length = 0;
    int j;
    for(j = 0; j < lists; j++)
      length += SortedList_length(&array[j]);
  }
    
  SortedListElement_t* node = NULL;
  for(i = start; i < end; i++){
    bucket = hash((char*)elements[i].key);
    if(mutex_locking){
      set_time(&threadStart_time);
      pthread_mutex_lock(&locks[bucket]);
      set_time(&threadEnd_time);
      thread_array[index] += get_time_lapse(&threadStart_time, &threadEnd_time);
      lookup_delete(node, bucket, i);
      pthread_mutex_unlock(&locks[bucket]);
    }
    else if(spin_locking){
      while(__sync_lock_test_and_set(&spin_locks[bucket], 1));
      lookup_delete(node, bucket, i);
      __sync_lock_release(&spin_locks[bucket]);
    }
    else{
      lookup_delete(node, bucket, i);
    }
  }
  return NULL;
}

void start_process(){
  pthread_t threads[num_threads];
  set_time(&start_time);
  int i;
  struct threadArgs* args;
  for(i = 0; i < num_threads; ++i){
    args = malloc(sizeof(struct threadArgs));
    args->end = (i+1)*iterations;
    args->index = i;
    pthread_create(&threads[i], NULL, thread_function, (void*)args);
  }
  for(i = 0; i < num_threads; i++)
    pthread_join(threads[i], NULL);
  if(mutex_locking){
    for(i = 0; i < lists; i++)
      pthread_mutex_destroy(&locks[i]);
  }
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
	locks = malloc(sizeof(pthread_mutex_t)*lists);
	int i;
	for(i = 0; i < lists; i++)
	  pthread_mutex_init(&locks[i], NULL);
	break;
      case 's':
	strcat(sync_name, "s");
	spin_locking = 1;
	spin_locks = malloc(sizeof(int)*lists);
	int j;
	for(j = 0; j < lists; ++j)
	  spin_locks[j] = 0;
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
  int i;
  for(i = 0; i < num_threads; i++)
    mutex_wait += thread_array[i];
  int operations = num_threads * iterations * 3;
  long long run_time = get_time_lapse(&start_time, &end_time);
  if(mutex_wait == 0)
    printf("%s,%d,%d,%d,%d,%lld,%lld\n",type,num_threads,iterations,
           lists,operations,run_time,run_time/operations);
  else
    printf("%s,%d,%d,%d,%d,%lld,%lld,%lld\n",type,num_threads,iterations,
	   lists,operations,run_time,run_time/operations,mutex_wait/operations);
}

void destroy(){
  free(test_name);
  int i;
  for(i = 0; i < num_threads*iterations; i++)
    free((void*)elements[i].key);
  free(elements);
  free(array);
  if (locks != NULL)
    free(locks);
  if (spin_locks != NULL)
    free(spin_locks);
  free(thread_array);
}

int main(int argc, char ** argv){
  checkargs(argc, argv);
  initialize_list();
  parse_arguments();
  start_process();
  int length = 0;
  int i;
  for(i = 0; i < lists; i++)
    length += SortedList_length(&array[i]);
  if(length != 0){
    error("Length of list is not 0 after completion");
  }
  write_out(test_name);
  destroy();
  exit(0);
}

