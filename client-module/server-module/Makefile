# Makefile for the application testing module
CC 	= gcc #arm-linux-androideabi-gcc -O2
LD	= ld
INCS 	= -I. 
DEFS 	=
OBJS	= monserver.o
LIBNAME	= .
CFLAGS 	= -g -Wall $(DEFS) $(INCS)
LIBS 	= .
	
#
all: monserver

%.o: %.c
	@rm -f $@
	$(CC) $(CFLAGS) -c -o $@ $(INCS) $<

# compile the test application
monserver: monserver.o
	$(CC) $(CFLAGS) -o monserver monserver.o

	
.PHONY : clean
clean:
	rm -f *.o monserver *~
