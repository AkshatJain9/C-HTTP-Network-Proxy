#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"
#include "cache.h"


int findResource(char* queryKey, char* bufferToFill) {
    if (!head) {
        return 0;
    }

    cached_obj* curr = head;

    // printf("Looking for: %s\n", queryKey);

    while (curr) {
        // printf("Current node has: %s\n", curr->key);
        if (!strcmp(queryKey, curr->key)) {
            if (replacement_policy) {
                cached_obj* currPrev = curr->prev;
                cached_obj* currNext = curr->next;

                curr->prev = NULL;
                curr->next = head;

                if (currPrev) {
                    currPrev->next = currNext;
                }
                if (currNext) {
                    currNext->prev = currPrev;
                }

                head = curr;

            } else {
                curr->lfuCount++;
            }

            printf("There was a cache hit!\n");
            char* toReturn = curr->html;
            strcat(bufferToFill, toReturn);
            // printf(toReturn); // Prints fine
            return 1;
            // return toReturn;
        }

        curr = curr->next;
    }

    return 0;
}

int addResource(char* queryKey, char* htmlToStore) {
    int sizeToAdd = strlen(queryKey) + strlen(htmlToStore) + 2;
    if (sizeToAdd > MAX_OBJECT_SIZE) {
        return 0;
    }
    while (cacheSize + sizeToAdd > 1049000) {
        // if (replacement_policy) {
            Free(end->key);
            Free(end->html);
            cacheSize -= end->size;
            cached_obj* newEnd = end->prev;
            Free(end);
            end = newEnd;
        // } else {

        // }
        
    }

    char* newKeyLoc = calloc(strlen(queryKey) + 1, 1);
    char* newHTMLLoc = calloc(strlen(htmlToStore) + 5, 1);

    strcpy(newKeyLoc, queryKey);
    strcpy(newHTMLLoc, htmlToStore);

    // printf("Storing the following:\n");
    // printf(newKeyLoc);
    // printf(newHTMLLoc);

    cached_obj* newCacheObj = malloc(sizeof(cached_obj));
    newCacheObj->key = newKeyLoc;
    newCacheObj->html = newHTMLLoc;
    newCacheObj->lfuCount = 0;
    newCacheObj->size = sizeToAdd;
    newCacheObj->htmlSize = strlen(htmlToStore) + 1;
    newCacheObj->prev = NULL;
    newCacheObj->next = head;

    cacheSize += sizeToAdd;

    if (!head) {
        head = newCacheObj;
        end = newCacheObj;
    } else {
        head = newCacheObj;
    }
    
    printf("String was added in Cache!\n");
    return 1;
}

