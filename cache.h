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
    char* html;
    int lfuCount;
    int size;
    int htmlSize;
    cached_obj* prev;
    cached_obj* next;
};

// Declare function for reading, and writing
int findResource(char* queryKey, char* bufferToFill);
int addResource(char* queryKey, char* htmlToStore, int hmtlSize);

// Keep track of actaul Cache in global variable
static cached_obj* head;
// static cached_obj* end;

// Meta-variables to keep track of size, LRU or LFU and an overall P-Thread lock
static int cacheSize = 0;
static int replacement_policy;
static pthread_rwlock_t lock;
static long globalTime = 0;