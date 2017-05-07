TARGET	= schedbench
LIB	= /usr/lib/resch/libresch.a
CC	= gcc
INCDIR	= ../include/	

default:
	$(CC) -o $(TARGET) -I$(INCDIR) -lm $(TARGET).c exec.c taskset.c $(LIB)

clean:
	rm -f  $(TARGET) config.h .error.log .util result *~
distclean:
	rm -f  $(TARGET) config.h .error.log .util result *~
	rm -fr taskset
