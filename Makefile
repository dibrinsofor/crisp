CC=gcc
CFLAGS = -Wall
CFLAGS += -std=c99
CFLAGS += -ledit
CFLAGS += -lm
repl: repl.o mpc.o

clean:
	rm -f repl.o mpc.o repl