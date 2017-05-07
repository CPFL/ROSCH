CFLAGS  = -W -Wall -g -O2 -s -pipe
INCDIR	= ../include
LIB 	= /usr/lib/resch/libresch.a

all: 
	gcc $(CFLAGS) -I$(INCDIR) -o test_overhead test_overhead.c $(LIB)

clean:
	rm -f  test_overhead *~
