all: programa


CFLAGS = -Wall -Wextra -pthread

ifdef DEBUG
CFLAGS += -ggdb3
else
CFLAGS += -O2
endif

programa:
	gcc $(CFLAGS) threads.c -o threads

clean:
	rm *.o
