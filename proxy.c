#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36\r\n";

int main(int argc, char **argv)
{
    // Generated Port 29722
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <replacement_policy>\n", argv[0]);
    }

    int listenfd = Open_listenfd(argv[1]);
    int tinyClientFd = Open_clientfd("localhost", argv[1]);
    char* replacement_policy = argv[2];


    
    Close(tinyClientFd);
    return 0;
}
