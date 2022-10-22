#ifndef COMP2310_CACHE_H
#define COMP2310_CACHE_H

#include <stdlib.h>
#include "csapp.h"

#endif

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

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



static cached_obj* head;
static cached_obj* end;
static int cacheSize = 0;

static int replacement_policy;

static pthread_rwlock_t lock;