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
	$(CC) -L"/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca" -Wall $(CFLAGS) -o $(BUILD)/archivo_de_swap_supermasivo archivo_de_swap_supermasivo.c -lShared_Library -lcommons -lpthread
	$(CC) -L"/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca" -Wall $(CFLAGS) -o $(BUILD)/audiencia audiencia.c -lShared_Library -lhilolay -lcommons -lpthread
	$(CC) -L"/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca" -Wall $(CFLAGS) -o $(BUILD)/caballeros_de_SisOp_Afinador caballeros_de_SisOp_Afinador.c -lShared_Library -lhilolay -lcommons -lpthread
	$(CC) -L"/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca" -Wall $(CFLAGS) -o $(BUILD)/caballeros_de_SisOp_Solo caballeros_de_SisOp_Solo.c -lShared_Library -lhilolay -lcommons -lpthread
	$(CC) -L"/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca" -Wall $(CFLAGS) -o $(BUILD)/estres_compartido estres_compartido.c -lShared_Library -lhilolay -lcommons -lpthread
	$(CC) -L"/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca" -Wall $(CFLAGS) -o $(BUILD)/estres_privado estres_privado.c -lShared_Library -lhilolay -lcommons -lpthread
	$(CC) -L"/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca" -Wall $(CFLAGS) -o $(BUILD)/recursiva recursiva.c -lShared_Library -lhilolay -lcommons -lpthread
	$(CC) -L"/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca" -Wall $(CFLAGS) -o $(BUILD)/revolucion_compartida revolucion_compartida.c -lShared_Library -lhilolay -lcommons -lpthread
	$(CC) -L"/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca" -Wall $(CFLAGS) -o $(BUILD)/revolucion_privada revolucion_privada.c -lShared_Library -lhilolay -lcommons -lpthread 

archivo_de_swap_supermasivo:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca ./$(BUILD)/archivo_de_swap_supermasivo

audiencia:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca ./$(BUILD)/audiencia

caballeros_de_SisOp_Afinador:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca ./$(BUILD)/caballeros_de_SisOp_Afinador

caballeros_de_SisOp_Solo:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca ./$(BUILD)/caballeros_de_SisOp_Solo

estres_compartido:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/Shared_Library/biblioteca ./$(BUILD)/estres_compartido

estres_privado:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./$(BUILD) ./$(BUILD)/estres_privado

recursiva:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./$(BUILD) ./$(BUILD)/recursiva

revolucion_compartida:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./$(BUILD) ./$(BUILD)/revolucion_compartida

revolucion_privada:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./$(BUILD) ./$(BUILD)/revolucion_privada
