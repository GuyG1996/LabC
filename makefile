CC = gcc
CFLAGS = -Wall -g -m32

all: myshell mypipeline looper

myshell: LineParser.o myshell.o 
	$(CC) $(CFLAGS) -o myshell myshell.o LineParser.o

looper: looper.o
	$(CC) $(CFLAGS) -o looper looper.o

mypipeline: mypipeline.o
	$(CC) $(CFLAGS) -o mypipeline mypipeline.o
	
LineParser.o: LineParser.c LineParser.h
	$(CC) $(CFLAGS) -c LineParser.c -o LineParser.o

myshell.o: myshell.c 
	$(CC) $(CFLAGS) -c myshell.c -o myshell.o

looper.o: looper.c
	$(CC) $(CFLAGS) -c looper.c -o looper.o

mypipeline.o: mypipeline.c
	$(CC) $(CFLAGS) -c mypipeline.c -o mypipeline.o

.PHONY: clean
	
clean:
	rm -f mypipeline myshell looper *.o