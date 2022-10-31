#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"
#include "cache.h"

// Keep track of actual Cache and replacement metrics
static cached_obj* head;
static int cacheSize = 0;
static long globalTime = 0;

/*
*  Given a Query Key, will fill the provided buffer if the resource is in the Cache
*/
int findResource(char* queryKey, void* bufferToFill) {
    // Set a read lock only, so multiple clients can access it
    pthread_rwlock_rdlock(&lock);
    if (!head) {
        pthread_rwlock_unlock(&lock);
        return 0;
    }

    // Search through each node in the linked list to find a matching key
    cached_obj* curr = head;

    printf("Looking for: %s\n", queryKey);

    while (curr) {
        if (!strcmp(queryKey, curr->key)) {
            // If there was a Cache hit, fill the buffer, and update the Replacement Metric
            printf("There was a cache hit!\n");
            memcpy(bufferToFill, curr->html, curr->size);

            // For LRU it is the time, for LFU it is the count
            if (replacement_policy) {
                curr->replacementMetric = globalTime++;
            } else {
                curr->replacementMetric++;
            }
            
            pthread_rwlock_unlock(&lock);
            return 1;
        }

        curr = curr->next;
    }

    // Always unlocking before returning to ensure no deadlocks
    pthread_rwlock_unlock(&lock);
    return 0;
}

/*
* Adds a resource, K,V pair to the front of the Cache, ensuring size conditions
*/
int addResource(char* queryKey, char* htmlToStore, int htmlSize) {
    // Make sure size fits criteria
    if (htmlSize > MAX_OBJECT_SIZE) {
        return 0;
    }

    // Allocate new memory locations on heap to copy strings
    char* newKeyLoc = calloc(strlen(queryKey) + 1, 1);
    void* newHTMLLoc = calloc(htmlSize + 1, 1);

    // Because proxy temporarily stores fields, copy them over to heap
    strcpy(newKeyLoc, queryKey);
    memcpy(newHTMLLoc, htmlToStore, htmlSize);

    // Set fields correctly
    cached_obj* newCacheObj = malloc(sizeof(cached_obj));
    newCacheObj->key = newKeyLoc;
    newCacheObj->html = newHTMLLoc;

    // Initialise according to replacement policy
    if (replacement_policy) {
        newCacheObj->replacementMetric = globalTime++;
    } else {
        newCacheObj->replacementMetric = 0;
    }
    
    newCacheObj->size = htmlSize;
    newCacheObj->prev = NULL;

    // Remove elements according the LRU/LFU policy until everything fits
    // Using a write lock to ensure safety 
    pthread_rwlock_wrlock(&lock);
    while (cacheSize + htmlSize > MAX_CACHE_SIZE) {
            cached_obj* curr = head;

            // Keep track of most approiate to remove so far
            int min = curr->replacementMetric;
            cached_obj* toRemove = head;

            // For each element, see if Metric is beaten, if so, replace toRemove
            while (curr) {
                if (curr->replacementMetric < min) {
                    min = curr->replacementMetric;
                    toRemove = curr;
                }

                curr = curr->next;
            }

            printf("Eviciting the followng: %s\n", toRemove->key);

            // Free up space on heap
            Free(toRemove->key);
            Free(toRemove->html);

            // If the toRemove is the head, update the head
            if (head == toRemove) {
                head = head->next;
                if (head) {
                    head->prev = NULL;
                }
            } else {
                // Otherwise just update the next and previous pointers
                cached_obj* toRemovePrev = toRemove->prev;
                cached_obj* toRemoveNext = toRemove->next;
                if (toRemovePrev) {
                    toRemovePrev->next = toRemoveNext;
                }
                if (toRemoveNext) {
                    toRemoveNext->prev = toRemovePrev;
                }
            }

            // Update size and formally remove Cache entry
            cacheSize -= toRemove->size;
            Free(toRemove);
        
    }
    
    // Add to front of Cache, with Write lock since we are doing a modification
    newCacheObj->next = head;
    cacheSize += htmlSize;

    // Updating header element as necessary
    head = newCacheObj;
    pthread_rwlock_unlock(&lock);
    
    printf("%s was added in Cache!\n", queryKey);
    return 1;
}

