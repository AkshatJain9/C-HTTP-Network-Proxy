#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_conn_hdr = "Proxy-Connection: close\r\n";
static const char *host = "Host: localhost:29723\r\n";

static const char *get_hdr_n = "GET ";
static const char *http_prot_hdr = " HTTP/1.0\r\n";

// Generated Port 29722 for proxy 29723 for tiny
int main(int argc, char **argv)
{
    // Check if input is in specified form
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <replacement_policy>\n", argv[0]);
    }

    // Capture port as String as well as replacement policy
    char* port = argv[1];
    char* replacement_policy = argv[2];

    // Convert String port to Tiny Port to Listen to
    int tinyPort = atoi(port) + 1;
    int length = snprintf(NULL, 0, "%d", tinyPort);
    char* tinyPortString = malloc(length + 1);
    snprintf(tinyPortString, length + 1, "%d", tinyPort);

    // Open listening instance to port
    int listenfd = Open_listenfd(port);

    // Act as a client to tiny
    int tinyClientFd = Open_clientfd("localhost", tinyPortString);// to send to client

    // For a specific client, we store connfd and store info in addr
    int connfd;
    struct sockaddr_storage clientaddr;

    // We need to store how long their message is as well as the actual message
    int n; 
    char buf[MAXLINE];
    // Read from connfd as buffer
    rio_t rio;

    // For reading response from tiny
    char tbuf[MAXLINE];
    rio_t trio;

    Rio_readinitb(&trio, tinyClientFd);
    
    int clientlen = sizeof(struct sockaddr_storage);


    char toSend[MAXLINE];


    while (1) {
        // Accepting listening -> connfd would be passed into echo
        connfd = Accept(listenfd, (SA*) &clientaddr, &clientlen);

        // Associate rio struct with server listener
        Rio_readinitb(&rio, connfd);

        Rio_readlineb(&rio, buf, MAXLINE);
        printf(buf);
        strcat(toSend, get_hdr_n);

        int slashCount = 0;

        int i = 0;
        int start;
        int end;

        while (buf[i]) {
            if (buf[i] == ' ' && slashCount == 3) {
                end = i;
                printf("%i\n", i);
                break;
            }

            if (buf[i] == '/') {
                slashCount++;
                if (slashCount == 3) {
                    start = i;
                    printf("%i\n", i);
                }
            }
            i++;
        }

        // printf(start);
        char* reso = malloc(end - start + 1);
        memset(reso, 0, end-start+1);

        memcpy(reso, &buf[start], end-start);

        strcat(toSend, reso);
        strcat(toSend, http_prot_hdr);

        strcat(toSend, host);
        strcat(toSend, user_agent_hdr);
        strcat(toSend, conn_hdr);
        strcat(toSend, prox_conn_hdr);
        

        // Loads data into buffer AS SERVER
        while((n = Rio_readlineb(&rio, buf, MAXLINE)) > 2) {
            if (!isHost(buf) && !isUsrAgent(buf) && !isConn(buf) && !isProxConn(buf)) {
                strcat(toSend, buf);
            }
        }
        
        strcat(toSend, "\r\n");

        printf("Sending the following\n");
        printf(toSend);
        
        // Write to tiny using CLIENT syntax
        Rio_writen(tinyClientFd, toSend, MAXLINE);

        printf("Message was sent! \n");
                    

        // // Read from tiny using CLIENT syntax
        // while (Rio_readlineb(&trio, tbuf, MAXLINE)) {
        //     printf(tbuf);
        // }

        

        // printf("Should have printed something \n");

        // nt = Rio_readlineb(&trio, tbuf, MAXLINE);

        // Rio_writen(connfd, tbuf, nt);

    }

    printf("Exiting Proxy\n");
    Close(tinyClientFd);
    return 0;
}

int isGet(char* buf) {
    return buf[0] == 'G' && buf[1] == 'E' && buf[2] == 'T';
}

int isHost(char* buf) {
    return buf[0] == 'H' && buf[1] == 'o' && buf[2] == 's' && buf[3] == 't';
}

int isUsrAgent(char* buf) {
    return buf[0] == 'U' && buf[1] == 's' && buf[2] == 'e' && buf[3] == 'r' && buf[4] == '-'
    && buf[5] == 'A' && buf[6] == 'g' && buf[7] == 'e' && buf[8] == 'n' && buf[9] == 't';
}

int isConn(char* buf) {
    return buf[0] == 'C' && buf[1] == 'o' && buf[2] == 'n' && buf[3] == 'n' && buf[4] == 'e'
    && buf[5] == 'c' && buf[6] == 't' && buf[7] == 'i' && buf[8] == 'o' && buf[9] == 'n';
}

int isProxConn(char* buf) {
    return buf[0] == 'P' && buf[1] == 'r' && buf[2] == 'o' && buf[3] == 'x' && buf[4] == 'y' 
    && buf[5] == '-' && buf[6] == 'C' && buf[7] == 'o' && buf[8] == 'n' && buf[9] == 'n' && buf[10] == 'e'
    && buf[11] == 'c' && buf[12] == 't' && buf[13] == 'i' && buf[14] == 'o' && buf[15] == 'n';
}

void getRes(char* buf, char* toSend) {
    

}
