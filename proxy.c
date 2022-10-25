#include <stdio.h>
#include "csapp.h"
#include "cache.h"

static const char* user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36\r\n";

void* handleRequest(void* vargp);
void returnErrortoClient(int fd);


// Generated Port 29722 for proxy 29723 for tiny
int main(int argc, char **argv)
{
    // Check if input is in specified form
    if (argc != 3 || strlen(argv[2]) != 3 || argv[2][0] != 'L' || 
    (argv[2][1] != 'R' && argv[2][1] != 'F') || (argv[2][2] != 'U')) {
        fprintf(stderr, "Usage: %s <port> <LRU | LFU>\n", argv[0]);
        exit(-1);
    }

    // Capture port as String as well as replacement policy, 1 for LRU, 0 for LFU
    char* port = argv[1];
    replacement_policy = (argv[2][1] == 'R');

    // Open listening instance to port
    int listenfd = Open_listenfd(port);

    // For a specific client, we store clientConnfd and store info in addr
    int* clientConnfd;
    struct sockaddr_storage clientaddr;
    unsigned int clientlen = sizeof(struct sockaddr_storage);
    
    // We also need a thread variable to initialise threads and a lock
    pthread_t tid;
    pthread_rwlock_init(&lock, 0);

    while (1) {
        // Accepting listening
        clientConnfd = malloc(sizeof(int));
        *clientConnfd = Accept(listenfd, (SA*) &clientaddr, &clientlen);

        Pthread_create(&tid, NULL, handleRequest, clientConnfd);
    }
    
    return 0;
}


void* handleRequest(void* vargp) {
    // Create connections and detach from main thread
    int clientConnfd = *((int*) vargp);
    Pthread_detach(pthread_self());
    Free(vargp);

    // Initialise buffers for reading and writing from client & tiny
    char clientBuffer[MAXLINE] = "";
    char serverBuffer[MAXLINE] = "";
    char serverToSend[MAXLINE] = "";
    void* toCacheHTML = calloc(MAX_OBJECT_SIZE, 1);
    // void toCacheHTML[MAX_OBJECT_SIZE] = "";

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

    while ((Rio_readlineb(&rio, clientBuffer, MAXLINE)) > 2) {
        // Firstline is always GET, so we process that first
        if (firstLine) {
            // Seperate into method and URL, and get just the host
            sscanf(clientBuffer, "%s %s", HTTPMethod, URL);

            if (strcmp("GET", HTTPMethod)) {
                printf(HTTPMethod);
                printf("\n");
                printf("Proxy only Implements GET Method, please try again!\n");
                // returnErrortoClient(clientConnfd);
                returnErrortoClient(clientConnfd);
                return NULL;
            }

            if (!strstr(URL, "http://")) {
                printf("URL to be requested must be HTTP!\n");
                // returnErrortoClient(clientConnfd);
                returnErrortoClient(clientConnfd);
                return NULL;
            }

            sscanf(URL, "http://%s", domainPortSplit);

            // If the host & resource is cached, just send it back
            if (findResource(domainPortSplit, toCacheHTML)) {
                printf("Returned the following:\n");
                printf(toCacheHTML);
                printf("\n");
                Rio_writen(clientConnfd, toCacheHTML, MAX_OBJECT_SIZE);
                Close(clientConnfd);
                return NULL;
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
                !strstr(clientBuffer, "Host: ")) {
                
                // Change to consistent HTTP format and contcatenate
                if ((oldHttp = strstr(clientBuffer, "HTTP/1.1"))) {
                    strncpy(oldHttp, "HTTP/1.0", strlen("HTTP/1.0") + 1);
                }

                strcat(serverToSend, clientBuffer);
            }
        }
    }

    // Close HTTP request with empty blank line
    strcat(serverToSend, "\r\n");

    printf("Built the message to send to the server\n");
    
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

    printf("Got the server connector\n");

    if (endServerFd < 0) {
        printf("Couldn't establish a connection to the Server!\n");
        returnErrortoClient(clientConnfd);

        return NULL;
    }

    // For reading response from the server
    rio_t trio = {};
    Rio_readinitb(&trio, endServerFd);

    printf("Will send the following\n");
    printf(serverToSend);

    // Write serverToSend to the tinyClient, meaning send complete HTTP request
    Rio_writen(endServerFd, serverToSend, MAXLINE);

    printf("Sent message to server\n");

    int ssize;
    int nsize = 0;

    int full = 1;
    // Read response from server into serverBuffer, then directly into buffer to send back to client
    while ((ssize = rio_readnb(&trio, serverBuffer, MAXLINE)) > 0) {
        printf("Got inside read loop\n");
        Rio_writen(clientConnfd, serverBuffer, ssize);

        if (full && ssize + nsize < MAX_OBJECT_SIZE) {
            memcpy(toCacheHTML + nsize, serverBuffer, ssize);
            nsize += ssize;
        } else {
            full = 0;
        }

        // strcat(toCacheHTML, serverBuffer);
        printf("%i\n", ssize);
    }

    printf("This is what we returned\n");
    printf(toCacheHTML);
    printf("\n");

    // Create a valid key by re-concatenation of host name, and with server buffer
    strcat(domainPortSplit, resourceToRequest);

    printf("Got a full key\n");

    if (full) {
        addResource(domainPortSplit, toCacheHTML, nsize);
        printf("Added to Cache\n");
    }
    
    Free(toCacheHTML);

    // Close client and Tiny connection
    Close(endServerFd);
    Close(clientConnfd);
    return NULL;
}


void returnErrortoClient(int fd) 
{

	char header[MAXLINE], body[MAXLINE];

	/* Build the HTTP response body */
	sprintf(body, "<html><title>Server Error</title>");
	sprintf(body, "%s400: Invalid Request\r\n", body);

	/* Print the HTTP response */
	sprintf(header, "HTTP/1.0 400 Invalid Request\r\n");
	Rio_writen(fd, header, strlen(header));
	sprintf(header, "Content-type: text/html\r\n");
	Rio_writen(fd, header, strlen(header));
	sprintf(header, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, header, strlen(header));
	Rio_writen(fd, body, strlen(body));
}
