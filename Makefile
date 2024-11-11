include ./Makefile.inc

all: main

main:
	$(CC) -o main main.c ./src/lib/*.c

server:


client:


clean:
	rm -rf src/*.o main


