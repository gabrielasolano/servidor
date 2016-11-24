all: programa limpa

CFLAGS = -Wall -Wextra

ifdef DEBUG
CFLAGS += -ggdb3
else
CFLAGS += -O2
endif

programa: recuperadorWeb.o
	gcc recuperadorWeb.o -o recuperadorWeb

recuperadorWeb.o : recuperadorWeb.c
	gcc -c $(CFLAGS) recuperadorWeb.c

limpa: 
	rm *.o
