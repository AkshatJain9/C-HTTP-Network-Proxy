#ifndef COMP2310_CACHE_H
#define COMP2310_CACHE_H

#include <stdlib.h>
#include "csapp.h"

#endif

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

// A cached object will be stored in the following
typedef struct cached_obj cached_obj;
struct cached_obj {
    char* key;
    void* html;
    int replacementMetric;
    int size;
    cached_obj* prev;
    cached_obj* next;
};

// Declare function for reading, and writing
int findResource(char* queryKey, void* bufferToFill);
int addResource(char* queryKey, char* htmlToStore, int hmtlSize);


// Meta-variables to keep track of LRU or LFU and an overall P-Thread lock
static int replacement_policy;
static pthread_rwlock_t lock;