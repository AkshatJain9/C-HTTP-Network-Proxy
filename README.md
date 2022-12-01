# C HTTP Proxy

This project implements a HTTP proxy to handle GET requests over the internet. To run this project, you must be on a POSIX compliant machine (e.g. Linux) or you can use WSL on Windows. This project was made without any external C libraries with the exception of `csapp.c`.

To run the proxy, run the following command in your terminal;

`make proxy`

 `./proxy <port> <LRU | LFU>`

Where LRU or LFU indicates the replacement policy for the included Cache.

This proxy can be used with a browser if set in default browser settings. 

The proxy has the following features;
- Dynamic Threadpool for handling concurrent clients using pthreads
- Usage of reading/writing locks to avoid the readers/writers problem and deadlocks
- Caching in Main Memory using either a Least Frequently Used (LFU) or Least Recently Used (LRU) policy
- Custom error handling
- Ability to handle all forms of media
