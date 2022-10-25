#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"
#include "cache.h"


int findResource(char* queryKey, void* bufferToFill) {
    pthread_rwlock_rdlock(&lock);
    if (!head) {
        pthread_rwlock_unlock(&lock);
        return 0;
    }

    cached_obj* curr = head;

    // printf("Looking for: %s\n", queryKey);

    while (curr) {
        // printf("Current node has: %s\n", curr->key);
        if (!strcmp(queryKey, curr->key)) {
            printf("There was a cache hit!\n");

            memcpy(bufferToFill, curr->html, curr->size);

            if (replacement_policy) {
                curr->lfuCount = globalTime++;
            } else {
                curr->lfuCount++;
            }
            
            pthread_rwlock_unlock(&lock);
            return 1;
        }

        curr = curr->next;
    }
    pthread_rwlock_unlock(&lock);
    return 0;
}

/*
* Adds a resource, K,V pair to the front of the Cache, ensuring size conditions
*/
int addResource(char* queryKey, char* htmlToStore, int htmlSize) {
    // Make sure size fits criteria
    // int sizeToAdd = strlen(queryKey) + strlen(htmlToStore) + 2;
    if (htmlSize > MAX_OBJECT_SIZE) {
        return 0;
    }

    // Remove elements according the LRU/LFU policy until everything fits
    // Using a write lock to ensure safety 
    pthread_rwlock_wrlock(&lock);
    while (cacheSize + htmlSize > MAX_CACHE_SIZE) {
            cached_obj* curr = head;

            int min = curr->lfuCount;
            cached_obj* toRemove = head;

            while (curr) {
                if (curr->lfuCount < min) {
                    min = curr->lfuCount;
                    toRemove = curr;
                }

                curr = curr->next;
            }

            cached_obj* toRemovePrev = toRemove->prev;
            cached_obj* toRemoveNext = toRemove->next;

            Free(toRemove->key);
            Free(toRemove->html);

            if (toRemovePrev) {
                toRemovePrev->next = toRemoveNext;
            }
            if (toRemoveNext) {
                toRemoveNext->prev = toRemovePrev;
            }

            cacheSize -= toRemove->size;

            Free(toRemove);
        
    }
    // Undo lock temporarily as we prepare new cache object
    pthread_rwlock_unlock(&lock);

    // Allocate new memory locations on heap to copy strings
    char* newKeyLoc = calloc(strlen(queryKey) + 1, 1);
    void* newHTMLLoc = calloc(htmlSize + 1, 1);

    strcpy(newKeyLoc, queryKey);
    memcpy(newHTMLLoc, htmlToStore, htmlSize);

    // Set fields correctly
    cached_obj* newCacheObj = malloc(sizeof(cached_obj));
    newCacheObj->key = newKeyLoc;
    newCacheObj->html = newHTMLLoc;

    if (replacement_policy) {
        newCacheObj->lfuCount = globalTime++;
    } else {
        newCacheObj->lfuCount = 0;
    }
    
    newCacheObj->size = htmlSize;
    newCacheObj->prev = NULL;

    pthread_rwlock_wrlock(&lock);
    newCacheObj->next = head;
    // Add to front of Cache, with Write lock since we are doing a modification

    cacheSize += htmlSize;

    // Updating header element as necessary
    head = newCacheObj;
    pthread_rwlock_unlock(&lock);
    
    printf("String was added in Cache!\n");
    return 1;
}

