#include <stdio.h>
#include "csapp.h"
#include "cache.h"



// Storing All settings that we know that we have to send
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36\r\n";

static char* tinyPortString;

void* handleRequest(void* vargp);
int isHost(char* clientBuffer);
int isUsrAgent(char* clientBuffer);
int isConn(char* clientBuffer);
int isProxConn(char* clientBuffer);

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

    // Convert String port to Tiny Port to Listen to
    int tinyPort = atoi(port) + 1;
    int length = snprintf(NULL, 0, "%d", tinyPort);
    tinyPortString = malloc(length + 1);
    snprintf(tinyPortString, length + 1, "%d", tinyPort);

    // Open listening instance to port
    int listenfd = Open_listenfd(port);

    // For a specific client, we store clientConnfd and store info in addr
    int* clientConnfd;
    struct sockaddr_storage clientaddr;
    unsigned int clientlen = sizeof(struct sockaddr_storage);
    
    // We also need a thread variable to initialis threads
    pthread_t tid;

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
    char clientToReceive[MAXLINE] = "";

    // For reading request from client
    rio_t rio = {};
    Rio_readinitb(&rio, clientConnfd);

    char host[MAXLINE] = "";
    char resourceToRequest[MAXLINE] = "";
    char HTTPMethod[MAXLINE] = "";
    char URL[MAXLINE] = "";

    char* oldHttp;
    int firstLine = 1;

    char* potentialCache;

    while ((Rio_readlineb(&rio, clientBuffer, MAXLINE)) > 2) {
        if (firstLine) {
            // printf(clientBuffer);
            // printf("\n");
            sscanf(clientBuffer, "%s %s", HTTPMethod, URL);
            sscanf(URL, "http://%s", host);

            if (findResource(host, clientToReceive)) {
                // printf("Returning here\n");

                // printf("CP-1?\n");
                // printf(potentialCache);
                // // printf(findResource(host));
                // fflush(stdout);
                // printf("CP0\n");
                // fflush(stdout);
                Rio_writen(clientConnfd, clientToReceive, strlen(clientToReceive) + 1);
                // printf("CP2\n");
                Close(clientConnfd);
                return NULL;
            }


            strcpy(resourceToRequest, strpbrk(host, "/"));

            strcat(serverToSend, "GET ");
            strcat(serverToSend, resourceToRequest);
            strcat(serverToSend, " HTTP/1.0\r\n");

            *strpbrk(host, "/") = 0;

            strcat(serverToSend, "Host: ");
            strcat(serverToSend, host);
            strcat(serverToSend, "\r\n");


            strcat(serverToSend, user_agent_hdr);

            strcat(serverToSend, "Connection: close\r\n");
            strcat(serverToSend, "Proxy-Connection: close\r\n");


            firstLine = 0;
        } else {
            if (!strstr(clientBuffer, "Proxy-Connection: ") && 
                !strstr(clientBuffer, "Connection: ") && !strstr(clientBuffer, "Host: ")) {

                    if((oldHttp = strstr(clientBuffer, "HTTP/1.1"))) {
				        strncpy(oldHttp, "HTTP/1.0", strlen("HTTP/1.0") + 1);
			        }

                    strcat(serverToSend, clientBuffer);
                }
        }


    }


    // Close HTTP request with empty blank line
    strcat(serverToSend, "\r\n");
    

    int tinyClientFd;
    char* t;
    if ((t = strpbrk(host, ":"))) {
        *t = 0;
        tinyClientFd = Open_clientfd(host, t+1);
        *t = ':';
    } else {
        tinyClientFd = Open_clientfd(host, "80");
    }


    // For reading response from tiny
    rio_t trio = {};
    Rio_readinitb(&trio, tinyClientFd);

    // printf("here is what we are sending\n");
    // printf(serverToSend);
    // printf("\n");
    // Write serverToSend to the tinyClient, meaning send complete HTTP request
    Rio_writen(tinyClientFd, serverToSend, MAXLINE);

    int ssize;
    // Read response from tiny into serverBuffer, then directly into buffer to send back to client
    while ((ssize = Rio_readnb(&trio, serverBuffer, MAXLINE)) > 0) {
        // printf("Loop here how many times?\n");
        // printf(serverBuffer);
        // printf("\n");
        Rio_writen(clientConnfd, serverBuffer, ssize);
        strcat(clientToReceive, serverBuffer);
    }

    printf("See this\n");
    printf(host);
    printf("\n");
    printf(resourceToRequest);
    printf("\n");
    printf("\n");

    printf("Concatenated it is:\n");
    strcat(host, resourceToRequest);
    printf(host);
    printf("\n");
    addResource(host, clientToReceive);

    // printf("This is what we are sending back to the client!\n");
    // printf(clientToReceive);

    // Write response back to client
    // Rio_writen(clientConnfd, clientToReceive, MAXLINE);

    // Close client and Tiny connection
    Close(tinyClientFd);
    Close(clientConnfd);
    return NULL;
}



// Check if string in buffer was specifying User-Agent
int isUsrAgent(char* clientBuffer) {
    return clientBuffer[0] == 'U' && clientBuffer[1] == 's' && clientBuffer[2] == 'e' && clientBuffer[3] == 'r' && clientBuffer[4] == '-'
    && clientBuffer[5] == 'A' && clientBuffer[6] == 'g' && clientBuffer[7] == 'e' && clientBuffer[8] == 'n' && clientBuffer[9] == 't';
}

// Check if string in buffer was specifying Connection
int isConn(char* clientBuffer) {
    return clientBuffer[0] == 'C' && clientBuffer[1] == 'o' && clientBuffer[2] == 'n' && clientBuffer[3] == 'n' && clientBuffer[4] == 'e'
    && clientBuffer[5] == 'c' && clientBuffer[6] == 't' && clientBuffer[7] == 'i' && clientBuffer[8] == 'o' && clientBuffer[9] == 'n';
}

// Check if string in buffer was specifying Proxy-Connection
int isProxConn(char* clientBuffer) {
    return clientBuffer[0] == 'P' && clientBuffer[1] == 'r' && clientBuffer[2] == 'o' && clientBuffer[3] == 'x' && clientBuffer[4] == 'y' 
    && clientBuffer[5] == '-' && clientBuffer[6] == 'C' && clientBuffer[7] == 'o' && clientBuffer[8] == 'n' && clientBuffer[9] == 'n' && clientBuffer[10] == 'e'
    && clientBuffer[11] == 'c' && clientBuffer[12] == 't' && clientBuffer[13] == 'i' && clientBuffer[14] == 'o' && clientBuffer[15] == 'n';
}

