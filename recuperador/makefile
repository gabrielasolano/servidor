all: programa limpa

programa: recuperadorWeb.o
	gcc recuperadorWeb.o -o recuperadorWeb

recuperadorWeb.o : recuperadorWeb.c
	gcc -c recuperadorWeb.c

limpa: 
	rm *.o
