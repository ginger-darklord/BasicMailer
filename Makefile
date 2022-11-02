#############################################################################################
# Makefile
#############################################################################################
# G++ is part of GCC (GNU compiler collection) and is a compiler best suited for C++
CC=g++

# Compiler Flags: https://linux.die.net/man/1/g++
#############################################################################################
# -g: produces debugging information (for gdb)
# -Wall: enables all the warnings
# -Wextra: further warnings
# -O: Optimizer turned on
# -std: c++ 17 is the latest stable version c++2a is the upcoming version
# -I: Add the directory dir to the list of directories to be searched for header files.
# -c: says not to run the linker
# -pthread: Add support for multithreading using the POSIX threads library. This option sets 
#           flags for both the preprocessor and linker. It does not affect the thread safety 
#           of object code produced by the compiler or that of libraries supplied with it. 
#           These are HP-UX specific flags.
#############################################################################################
CFLAGS=-g -Wall -Wextra -O -std=c++2a -I /usr/local/include/gtest/ -pthread
GTEST=/usr/local/lib/libgtest.a

rebuild: clean all
all: server client

clean:
	clear
	rm -f *.o

myclient.o: client.cpp
	${CC} ${CFLAGS} -o myclient.o client.cpp -c

myserver.o: main.cpp
	${CC} ${CFLAGS} -o myserver.o main.cpp -c 

server: myserver.o
	${CC} ${CFLAGS} -o server myserver.o

client: myclient.o
	${CC} ${CFLAGS} -o client myclient.o
