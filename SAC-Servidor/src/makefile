#
# This is an example Makefile for a countwords program.  This
# program uses both the scanner module and a counter module.
# Typing 'make' or 'make count' will create the executable file.
#

# define some Makefile variables for the compiler and compiler flags
# to use Makefile variables later in the Makefile: $()
#
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#
# for C++ define  CC = g++
CC = gcc
CFLAGS  = -Wall -g -fmessage-length=0 -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags --libs`
INCLUDES = -I"/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library"  -I"/home/utnso/workspace/commons"
LFLAGS = -L"/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca" -L"/home/utnso/workspace/commons/Debug"
LIBS = -lShared_Library -lrt -lm -lreadline -lpthread -lcommons -lhilolay

# typing 'make' will invoke the first target entry in the file 
# (in this case the default target entry)
# you can name this target entry anything, but "default" or "all"
# are the most commonly used names by convention
#
all: SAC

# To create the executable file count we need the object files
# countwords.o, counter.o, and scanner.o:
#
SAC:  SAC-Servidor.c Servidor.h
	$(CC) $(CFLAGS) $(INCLUDES) -o SAC SAC-Servidor.c $(LFLAGS) $(LIBS)

# To create the object file countwords.o, we need the source
# files countwords.c, scanner.h, and counter.h:
#

# To start over from scratch, type 'make clean'.  This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean: 
	$(RM) SAC *.o *~