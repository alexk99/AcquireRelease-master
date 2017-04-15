CC=gcc
CFLAGS=-I. -std=gnu99
DEPS = main.h
OBJ = main.o
LIBS = -lpthread

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ)
	gcc $(LIBS) -o $@ $^ $(CFLAGS)