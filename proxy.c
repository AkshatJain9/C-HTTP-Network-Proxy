#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36\r\n";

// Generated Port 29722 for proxy 29722 for tiny
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
    int length = snprintf( NULL, 0, "%d", tinyPort );
    char* tinyPortString = malloc(length + 1);
    snprintf( tinyPortString, length + 1, "%d", tinyPort );

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

    char tbuf[MAXLINE];
    rio_t trio;
    
    int clientlen;

    while (1) {
        clientlen = sizeof(struct sockaddr_storage);

        // Accepting listening -> connfd would be passed into echo
        connfd = Accept(listenfd, (SA*) &clientaddr, &clientlen);
        // Associate rio struct with server listener
        Rio_readinitb(&rio, connfd);


        // Rio_readinitb(&trio, tinyClientFd);

        // tinyconnfd = Accept(tinyListedFd, (SA*) &tinyClientaddr, &clientlen);
        
        // Rio_readinitb(&trio, tinyconnfd);

        // Loads data into buffer AS SERVER
        while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
            printf("server received %d bytes\n", n);

            printf("Sending the following request: \n");
            printf(buf);

            // Write to tiny using CLIENT syntax
            Rio_writen(tinyClientFd, buf, n);

            printf("Message was sent! \n");
            

            // // Read from tiny using CLIENT syntax
            // while (Rio_readlineb(&trio, tbuf, MAXLINE)) {
            //     printf(tbuf);
            // }

            

            // printf("Should have printed something \n");

            // nt = Rio_readlineb(&trio, tbuf, MAXLINE);

            // Rio_writen(connfd, tbuf, nt);

        }

    }

    printf("Exiting Proxy\n");
    Close(tinyClientFd);
    return 0;
}
