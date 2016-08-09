CFLAGS=-O2 -std=c11 -s
LFLAGS=-lcrypto -lpthread

default: main

onion.o: onion.c
	gcc ${CFLAGS} -c onion.c
rsa.o: rsa.c
	gcc ${CFLAGS} -c rsa.c
search.o: search.c
	gcc ${CFLAGS} -c search.c
main: onion.o search.o rsa.o main.c
	gcc ${CFLAGS} -o main main.c onion.o search.o rsa.o ${LFLAGS}
clean:
	-rm *.o main
