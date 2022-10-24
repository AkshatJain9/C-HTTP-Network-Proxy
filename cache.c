#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"
#include "cache.h"


int findResource(char* queryKey, char* bufferToFill) {
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

            strcat(bufferToFill, curr->html);

            if (replacement_policy) {
                curr->lfuCount = globalTime++;
                // cached_obj* currPrev = curr->prev;
                // cached_obj* currNext = curr->next;

                // curr->prev = NULL;
                // curr->next = head;

                // if (currPrev) {
                //     currPrev->next = currNext;
                // }
                // if (currNext) {
                //     currNext->prev = currPrev;
                // }

                // head = curr;

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
int addResource(char* queryKey, char* htmlToStore) {
    // Make sure size fits criteria
    int sizeToAdd = strlen(queryKey) + strlen(htmlToStore) + 2;
    if (sizeToAdd > MAX_OBJECT_SIZE) {
        return 0;
    }

    // Remove elements according the LRU/LFU policy until everything fits
    // Using a write lock to ensure safety 
    pthread_rwlock_wrlock(&lock);
    while (cacheSize + sizeToAdd > MAX_CACHE_SIZE) {
        // if (replacement_policy) {
        //     Free(end->key);
        //     Free(end->html);
        //     cacheSize -= end->size;
        //     cached_obj* newEnd = end->prev;
        //     Free(end);
        //     end = newEnd;
        // } else {
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

            // TODO FREE TOREMOVE
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
        // }
        
    }
    // Undo lock temporarily as we prepare new cache object
    pthread_rwlock_unlock(&lock);

    // Allocate new memory locations on heap to copy strings
    char* newKeyLoc = calloc(strlen(queryKey) + 1, 1);
    char* newHTMLLoc = calloc(strlen(htmlToStore) + 1, 1);

    strcpy(newKeyLoc, queryKey);
    strcpy(newHTMLLoc, htmlToStore);

    // Set fields correctly
    cached_obj* newCacheObj = malloc(sizeof(cached_obj));
    newCacheObj->key = newKeyLoc;
    newCacheObj->html = newHTMLLoc;

    if (replacement_policy) {
        newCacheObj->lfuCount = globalTime++;
    } else {
        newCacheObj->lfuCount = 0;
    }
    
    newCacheObj->size = sizeToAdd;
    newCacheObj->htmlSize = strlen(htmlToStore) + 1;
    newCacheObj->prev = NULL;

    pthread_rwlock_wrlock(&lock);
    newCacheObj->next = head;
    // Add to front of Cache, with Write lock since we are doing a modification

    cacheSize += sizeToAdd;

    // Updating header element as necessary
    head = newCacheObj;
    pthread_rwlock_unlock(&lock);
    
    printf("String was added in Cache!\n");
    return 1;
}

