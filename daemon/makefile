all: programa clean

CFLAGS = -Wall -Wextra

ifdef DEBUG
CFLAGS += -ggdb3
else
CFLAGS += -O2
endif

programa: servidor.o
	gcc servidor.o -o servidor

servidor.o : servidor.c
	gcc -c $(CFLAGS) servidor.c

clean: 
	rm *.o
