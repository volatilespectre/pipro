CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c17

%.o: %.c
	$(CC) $^ -c -o $@ $(CFLAGS)


pipedebug: main.o
	$(CC) -o pipedebug main.o $(CFLAGS)
