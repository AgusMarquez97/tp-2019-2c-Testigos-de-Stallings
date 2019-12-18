UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CFLAGS := -D_XOPEN_SOURCE -Wno-deprecated-declarations
endif

CC := gcc
NAME := hilolayex
BUILD=bin

all: clean $(NAME).so

clean:
	$(RM) *.o
	$(RM) *.so
	$(RM) -r bin/
	mkdir -p bin

$(NAME).o:
	$(CC) -c -Wall $(CFLAGS) -fpic hilolay_alumnos.c -o $(BUILD)/$(NAME).o -lhilolay

$(NAME).so: $(NAME).o
	$(CC) -shared -o $(BUILD)/lib$(NAME).so $(BUILD)/$(NAME).o -lhilolay

entrega:

	$(CC) -L"/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca" -Wall $(CFLAGS) -o $(BUILD)/revolucion_compartida revolucion_compartida.c -lShared_Library -lhilolay -lcommons -lpthread

revolucion_compartida:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca ./$(BUILD)/revolucion_compartida

