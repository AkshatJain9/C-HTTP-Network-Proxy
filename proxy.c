#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

// Storing All settings that we know that we have to send
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_conn_hdr = "Proxy-Connection: close\r\n";
static const char *host = "Host: localhost:29723\r\n";

// Because GET req. from Proxy to Server is consistent, we will store prefix & suffix
static const char *get_hdr_n = "GET ";
static const char *http_prot_hdr = " HTTP/1.0\r\n";


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
    int replacement_policy = (argv[2][1] == 'R');

    // Convert String port to Tiny Port to Listen to
    int tinyPort = atoi(port) + 1;
    int length = snprintf(NULL, 0, "%d", tinyPort);
    tinyPortString = malloc(length + 1);
    snprintf(tinyPortString, length + 1, "%d", tinyPort);

    // Open listening instance to port
    int listenfd = Open_listenfd(port);

    // For a specific client, we store connfd and store info in addr
    int* connfd;
    struct sockaddr_storage clientaddr;
    unsigned int clientlen = sizeof(struct sockaddr_storage);
    
    // We also need a thread variable to initialis threads
    pthread_t tid;

    while (1) {
        // Accepting listening
        connfd = malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA*) &clientaddr, &clientlen);

        Pthread_create(&tid, NULL, handleRequest, connfd);
    }
    
    return 0;
}


void* handleRequest(void* vargp) {
    // Create connections and detach from main thread
    int connfd = *((int*) vargp);
    int tinyClientFd = Open_clientfd("localhost", tinyPortString);// to send to client
    Pthread_detach(pthread_self());
    Free(vargp);

    // Initialise buffers for reading and writing from client & tiny
    char clientBuffer[MAXLINE] = "";
    char serverBuffer[MAXLINE] = "";
    char serverToSend[MAXLINE] = "";
    char clientToReceive[MAXLINE] = "";

    // For reading request from client
    rio_t rio = {};
    Rio_readinitb(&rio, connfd);

    // For reading response from tiny
    rio_t trio = {};
    Rio_readinitb(&trio, tinyClientFd);

    // HTTP requests always start with GET, so we will process that first
    Rio_readlineb(&rio, clientBuffer, MAXLINE);
    strcat(serverToSend, get_hdr_n);

    // To get requested resource, look for string after third slash in URL
    int slashCount = 0;
    int i = 0;
    int start;
    int end;

    // Iterate over buffer
    while (clientBuffer[i]) {
        // If we have seen 3 slahes, then the URL portion is complete, break
        if (clientBuffer[i] == ' ' && slashCount == 3) {
            end = i;
            break;
        }

        // Count number of slahes seen, resource start is after third
        if (clientBuffer[i] == '/') {
            slashCount++;
            if (slashCount == 3) {
                start = i;
            }
        }
        i++;
    }

    // Allocate new space for the resource string, ensuring space for null terminator
    char* reso = malloc(end - start + 1);
    memset(reso, 0, end - start + 1);

    // Copy string into new space, concat to 'serverToSend' string and free original space
    memcpy(reso, &clientBuffer[start], end-start);
    strcat(serverToSend, reso);
    Free(reso);

    // Add final concluding string for GET request
    strcat(serverToSend, http_prot_hdr);

    // Concatenate all other necessary configs that were stored statically
    strcat(serverToSend, host);
    strcat(serverToSend, user_agent_hdr);
    strcat(serverToSend, conn_hdr);
    strcat(serverToSend, prox_conn_hdr);

    // Load all other non-mandatory settings into serverToSend buffer
    while((Rio_readlineb(&rio, clientBuffer, MAXLINE)) > 2) {
        if (!isHost(clientBuffer) && !isUsrAgent(clientBuffer) && !isConn(clientBuffer) && !isProxConn(clientBuffer)) {
            strcat(serverToSend, clientBuffer);
        }
    }
    
    // Close HTTP request with empty blank line
    strcat(serverToSend, "\r\n");
    
    // Write serverToSend to the tinyClient, meaning send complete HTTP request
    Rio_writen(tinyClientFd, serverToSend, MAXLINE);


    // Read response from tiny into serverBuffer, then directly into buffer to send back to client
    while (Rio_readlineb(&trio, serverBuffer, MAXLINE)) {
        strcat(clientToReceive, serverBuffer);
    }

    // Write response back to client
    Rio_writen(connfd, clientToReceive, MAXLINE);

    // Close client and Tiny connection
    Close(connfd);
    Close(tinyClientFd);
    return NULL;
}


// Check if string in buffer was specifying Host
int isHost(char* clientBuffer) {
    return clientBuffer[0] == 'H' && clientBuffer[1] == 'o' && clientBuffer[2] == 's' && clientBuffer[3] == 't';
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

