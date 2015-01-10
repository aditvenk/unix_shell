#Compiler is gcc
COMPILER=gcc
CFLAGS=-O -Wall

all: clean mysh

mysh: mysh.c
	$(COMPILER) $(CFLAGS) -o mysh mysh.c

clean: 
	rm -f mysh

debug: clean
	$(COMPILER) -Wall -O0 -ggdb -g -o mysh mysh.c
