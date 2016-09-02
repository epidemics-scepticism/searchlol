CFLAGS=-O2 -std=gnu99 -pthread -s
LFLAGS=-lcrypto

default: main

onion.o: onion.c
	gcc ${CFLAGS} -c $^
rsa.o: rsa.c
	gcc ${CFLAGS} -c $^
search.o: search.c
	gcc ${CFLAGS} -c $^
lock.o: lock.c
	gcc ${CFLAGS} -c $^
pronounce.o: pronounce.c
	gcc ${CFLAGS} -c $^
main: onion.o search.o rsa.o lock.o pronounce.o main.c
	gcc ${CFLAGS} -o main $^ ${LFLAGS}
clean:
	-rm *.o main
