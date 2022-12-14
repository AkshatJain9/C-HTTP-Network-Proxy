#include <stdio.h>
#include "csapp.h"
#include "cache.h"
#include "sbuf.h"

// Shared Buffer Attributes
#define NTHREADS 24
#define SBUFSIZE 128
sbuf_t sbuf;

static const char* user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36\r\n";

void handleRequest(int clientConnfd);
void returnErrortoClient(int fd, char* error_msg);
void* thread(void* vargp);


// Generated Port 29722 from "./port-for-user.pl u7284072"
int main(int argc, char **argv)
{
    // Check if input is in specified form
    if (argc != 3 || (strcmp(argv[2], "LRU") && strcmp(argv[2], "LFU"))) {
        fprintf(stderr, "Usage: %s <port> <LRU | LFU>\n", argv[0]);
        exit(-1);
    }

    // Capture port as String as well as replacement policy, 1 for LRU, 0 for LFU
    char* port = argv[1];
    replacement_policy = (argv[2][1] == 'R');

    // Open listening instance to port
    int listenfd = Open_listenfd(port);

    // For a specific client, we store clientConnfd and store info in addr
    struct sockaddr_storage clientaddr;
    unsigned int clientlen = sizeof(struct sockaddr_storage);
    
    // We also need a thread variable to initialise threads and a lock
    pthread_t tid[NTHREADS];
    pthread_rwlock_init(&lock, 0);
    
    // Initialise Shared Buffer
    sbuf_init(&sbuf, SBUFSIZE);

    // Create threads to pull from shared buffer
    for (int i = 0; i < NTHREADS; i++) {
        Pthread_create(&tid[i], NULL, thread, NULL);  
    }
	
    // Add client connection Fd's to shared buffer to be pulled
    int clientConnfd;
    while (1) {
        // Accepting listening
        clientConnfd = Accept(listenfd, (SA*) &clientaddr, &clientlen);
        sbuf_insert(&sbuf, clientConnfd);
    }
    
    return 0;
}

/*
*  Thread routing to pull client Fd's from shared buffer and start response routine
*/
void* thread(void* vargp) {
    Pthread_detach(pthread_self());
    while (1) {
        int clientConnfd = sbuf_remove(&sbuf);
        handleRequest(clientConnfd);
    }
    return NULL;
}

/*
*  Routine to handle a request from a client, and to return a response
*/
void handleRequest(int clientConnfd) {
    // Initialise buffers for reading and writing from client & tiny
    char clientBuffer[MAXLINE] = "";
    char serverBuffer[MAXLINE] = "";
    char serverToSend[MAXLINE] = "";
    void* toCacheHTML = calloc(MAX_OBJECT_SIZE, 1);

    // For reading request from client
    rio_t rio = {};
    Rio_readinitb(&rio, clientConnfd);

    // Initialise variables to be red from first line of request
    char domainPortSplit[MAXLINE] = "";
    char resourceToRequest[MAXLINE] = "";
    char HTTPMethod[MAXLINE] = "";
    char URL[MAXLINE] = "";

    // Initialise placeholderr variables
    char* oldHttp;
    int firstLine = 1;
    int errc;

    while ((errc = rio_readlineb(&rio, clientBuffer, MAXLINE)) > 2) {
        // Firstline is always GET, so we process that first
        if (firstLine) {
            // Seperate into method and URL, and get just the host
            sscanf(clientBuffer, "%s %s", HTTPMethod, URL);

            // Check if request is GET and it is a HTTP URL
            if (strcmp("GET", HTTPMethod)) {
                printf("Proxy only Implements GET Method, %s was attempted\n", HTTPMethod);
                returnErrortoClient(clientConnfd, "501 Non-Get Method");
                Close(clientConnfd);
                return;
            }

            if (!strstr(URL, "http://")) {
                printf("Protocol to be requested must be HTTP!\n");
                returnErrortoClient(clientConnfd, "501 Protocol must be HTTP!");
                Close(clientConnfd);
                return;
            }

            // Get just the host, port and resource in its own string
            sscanf(URL, "http://%s", domainPortSplit);

            // If the host & resource is cached, just send it back
            if (findResource(domainPortSplit, toCacheHTML)) {
                if (rio_writen(clientConnfd, toCacheHTML, MAX_OBJECT_SIZE) < 0) {
                    printf("Error returning Cached object\n");
                    returnErrortoClient(clientConnfd, "500 Error returning Cached object");
                }
                Close(clientConnfd);
                return;
            }

            // Copy the resource into its own string
            strcpy(resourceToRequest, strpbrk(domainPortSplit, "/"));

            // Make first line just a request of this resource as required
            strcat(serverToSend, "GET ");
            strcat(serverToSend, resourceToRequest);
            strcat(serverToSend, " HTTP/1.0\r\n");

            // To only process the host name, null terminate at /
            *strpbrk(domainPortSplit, "/") = 0;

            // Add just the hostname, which now terminates currectly
            strcat(serverToSend, "Host: ");
            strcat(serverToSend, domainPortSplit);
            strcat(serverToSend, "\r\n");

            // Concatnate the defaults we needed to send
            strcat(serverToSend, user_agent_hdr);
            strcat(serverToSend, "Connection: close\r\n");
            strcat(serverToSend, "Proxy-Connection: close\r\n");

            // Indicate the first line has been processed
            firstLine = 0;
        } else {
            // If the line isn't any of the template fields, process it
            if (!strstr(clientBuffer, "Proxy-Connection: ") && 
                !strstr(clientBuffer, "Connection: ") && 
                !strstr(clientBuffer, "Host: ") &&
                !strstr(clientBuffer, "User-Agent: ")) {
                
                // Change to consistent HTTP format and contcatenate
                if ((oldHttp = strstr(clientBuffer, "HTTP/1.1"))) {
                    strncpy(oldHttp, "HTTP/1.0", strlen("HTTP/1.0") + 1);
                }

                strcat(serverToSend, clientBuffer);
            }
        }
    }

    if (errc < 0) {
        printf("Error while reading request from Client\n");
        Close(clientConnfd);
        return;
    }

    // Close HTTP request with empty blank line
    strcat(serverToSend, "\r\n");
    
    // Because no cache, we have to open a sever connection
    int endServerFd;
    char* t;

    // Open by specified port, otherwise default on port 80
    if ((t = strpbrk(domainPortSplit, ":"))) {
        *t = 0;
        endServerFd = open_clientfd(domainPortSplit, t + 1);
        *t = ':';
    } else {
        endServerFd = open_clientfd(domainPortSplit, "80");
    }

    // If request was malformed, we can't continue so return
    if (endServerFd < 0) {
        printf("Couldn't establish a connection to the Server!\n");
        returnErrortoClient(clientConnfd, "503 Server Requested Does Not Exist or is Unavailable");
        Close(clientConnfd);
        return;
    }

    // For reading response from the server
    rio_t trio = {};
    Rio_readinitb(&trio, endServerFd);

    // Write serverToSend to the tinyClient, meaning send complete HTTP request
    if (rio_writen(endServerFd, serverToSend, MAXLINE) < 0) {
        printf("Error forwarding request to server\n");
        returnErrortoClient(clientConnfd, "500 Response could not be sent to server");
        Close(endServerFd);
        Close(clientConnfd);
        return;
    }

    // Keep track of current read's size, as well as overall size
    int currReadSize;
    int responseSize = 0;

    // Keep track if we can fit the full response back or not
    int full = 1;

    // Read response from server into serverBuffer, then directly into buffer to send back to client
    while ((currReadSize = rio_readnb(&trio, serverBuffer, MAXLINE)) > 0) {
        if (rio_writen(clientConnfd, serverBuffer, currReadSize) < 0) {
            printf("Error while sending back response to Client");
            returnErrortoClient(clientConnfd, "500 Server Stopped Transmitting Response");
            Close(endServerFd);
            Close(clientConnfd);
            return;
        }

        // If we still have room in the cache entry, then add it to the temp entry
        if (full && currReadSize + responseSize < MAX_OBJECT_SIZE) {
            memcpy(toCacheHTML + responseSize, serverBuffer, currReadSize);
            responseSize += currReadSize;
        } else {
            full = 0;
        }
    }

    // If we were able to cache the whole object, make a valid key and place in cache
    if (full) {
        strcat(domainPortSplit, resourceToRequest);
        addResource(domainPortSplit, toCacheHTML, responseSize);
    }
    
    // Because temporary cache was malloced, we free it
    Free(toCacheHTML);

    // Close client and Tiny connection
    Close(endServerFd);
    Close(clientConnfd);
}

/*
* Given a client's FD, returns a web-page stating the request was incorrect
*/
void returnErrortoClient(int fd, char* error_msg) {
	char header[MAXLINE], body[MAXLINE];

	/* Build the HTTP response body */
	sprintf(body, "<html><title>Server Error</title>");
    sprintf(body, "%s\r\n", error_msg);

	/* Print the HTTP response */
	sprintf(header, "HTTP/1.0 400 Invalid Request\r\n");
	Rio_writen(fd, header, strlen(header));
	sprintf(header, "Content-type: text/html\r\n");
	Rio_writen(fd, header, strlen(header));
	sprintf(header, "Content-length: %d\r\n\r\n", (int) strlen(body));
	Rio_writen(fd, header, strlen(header));
	Rio_writen(fd, body, strlen(body));
}
