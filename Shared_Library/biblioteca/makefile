# Makefile template for shared library

CC = gcc # C compiler

CFLAGS = -fPIC -Wall -Wextra -O2 -g # C flags
LDFLAGS = -shared  # linking flags
RM = rm -f  # rm command

TARGET_LIB = libShared_Library.so # target lib

SRCS = levantarConfig.c serializacion.c sockets.c mensajes.c logs.c utils.c libmuse.c mensajesMuse.c mensajesFuse.c mensajesSuse.c # source files
OBJS = $(SRCS:.c=.o)

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^

$(SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@

include $(SRCS:.c=.d)

.PHONY: clean
clean:
	-${RM} ${TARGET_LIB} ${OBJS} $(SRCS:.c=.d)
