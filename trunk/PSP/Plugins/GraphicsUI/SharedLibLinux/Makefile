CC=gcc
PNAME=SharedLibLinux.a

LIBS=
INC=-I.

CFLAGS=$(INC) -O1 -Wall -g -DLINUX

OBJS = Logging.o iniparser.o

all: $(OBJS)
	ar rcs $(PNAME) $(LIBS) $(OBJS)

.cpp.o:
	$(CC) $(CFLAGS) -c $*.cpp

clean:
	rm -f $(OBJS)
	rm -f $(PNAME)
