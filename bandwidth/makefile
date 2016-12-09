all: programa clean

CFLAGS = -Wall -Wextra

ifdef DEBUG
CFLAGS += -ggdb3
else
CFLAGS += -O2
endif

programa: servidor_bandwidth.o
	gcc servidor_bandwidth.o -o servidor_bandwidth

servidor_bandwidth.o : servidor_bandwidth.c
	gcc -c $(CFLAGS) servidor_bandwidth.c

clean: 
	rm *.o
