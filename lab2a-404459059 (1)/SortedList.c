#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "SortedList.h"
#include <string.h>
#include <pthread.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
    SortedListElement_t* cur = list->next;
    int inserted = 0;
    SortedListElement_t* prev = list;
    while(cur != NULL){
        if(strcmp(cur->key, element->key)>0){
            element->next = cur;
            element->prev = prev;
            prev->next = element;
            cur->prev = element;
            inserted = 1;
            break;
        }
        if(opt_yield & INSERT_YIELD)
            sched_yield();
        prev = cur;
        cur = cur->next;
    }
    if(!inserted){
        prev->next = element;
        element->prev = prev;
        element->next = NULL;
    }
}

int SortedList_delete(SortedListElement_t *element){
    if(element == NULL)
    {
        perror("Element to be deleted is NULL");
        return 1;
    }
    if(element->prev == NULL)
    {
        perror("Element's prev pointer is null, this is incorrect as head should always be in list");
        return 1;
    }
    if(element->prev->next != element)
    {
        perror("Corrupted prev pointer");
        return 1;
    }
    if(element->next != NULL && element->next->prev != element)
    {
        perror("Corrupted next pointer");
        return 1;
    }
    if(opt_yield & DELETE_YIELD)
        sched_yield();
    element->prev->next = element->next;
    if(element->next != NULL)
        element->next->prev = element->prev;
    element->next = NULL;
    element->prev = NULL;
    return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
    if(list == NULL || list->next == NULL)
        return NULL;
    SortedList_t* cur = list->next;
    while(cur != NULL){
        if(strcmp(key, cur->key)==0)
            return cur;
        if(opt_yield & LOOKUP_YIELD)
            sched_yield();
        cur = cur->next;
    }
    return NULL;
}

int SortedList_length(SortedList_t *list){
    if(list == NULL)
        return -1;
    if(list->next == NULL)
        return 0;
    int length = 0;
    SortedListElement_t* cur = list->next;
    while(1){
        if(opt_yield & LOOKUP_YIELD)
            sched_yield();
        if(cur->prev->next != cur)
            return -1;
        if(cur->next == NULL){
            return ++length;
        }
        if(cur->next->prev != cur){
            return -1;
        }
        length++;
        cur = cur->next;
    }
}

















