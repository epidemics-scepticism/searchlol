CFLAGS=-O2 -std=gnu11 -pthread -s
LFLAGS=-lcrypto

default: main

onion.o: onion.c
	gcc ${CFLAGS} -c onion.c
rsa.o: rsa.c
	gcc ${CFLAGS} -c rsa.c
search.o: search.c
	gcc ${CFLAGS} -c search.c
lock.o: lock.c
	gcc ${CFLAGS} -c lock.c
main: onion.o search.o rsa.o lock.o main.c
	gcc ${CFLAGS} -o main main.c onion.o search.o rsa.o lock.o ${LFLAGS}
clean:
	-rm *.o main
