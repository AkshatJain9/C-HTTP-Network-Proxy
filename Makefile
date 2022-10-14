CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

OBJS = proxy.o csapp.o

all: proxy tiny

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c  
	$(CC) $(CFLAGS) -c proxy.c

cache.o: cache.c cache.h
	$(CC) $(CFLAGS) -c cache.c

proxy: proxy.o cache.o csapp.o
	$(CC) $(CFLAGS) proxy.o cache.o csapp.o -o proxy $(LDFLAGS)

tiny:
	(cd tiny; make clean; make)
	(cd tiny/cgi-bin; make clean; make)

clean:
	rm -f *~ *.o proxy core 
	(cd tiny; make clean)
	(cd tiny/cgi-bin; make clean)
