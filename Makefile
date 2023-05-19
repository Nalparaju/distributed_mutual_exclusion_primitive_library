CC = gcc
CFLAGS = -Wall -g  -pedantic -std=c99
LDFLAGS = -lpthread

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

bank: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o bank