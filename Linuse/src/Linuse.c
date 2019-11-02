#include "Linuse.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {

	muse_init(getpid(),"127.0.0.1",5003);

	uint32_t pos = muse_alloc(50);

	
	char* msj = malloc(strlen("Se_recibió_la_siguiente_dirección:_9999999999999999999999"));
	sprintf(msj, "Se recibió la siguiente dirección: %u\n", pos);
	loggearInfo(msj);
	free(msj);

	char* cadena_cpy = "Cadena con algo";
	muse_cpy(pos, cadena_cpy, strlen(cadena_cpy) + 1);

	char* cadena_get = malloc(30);
	muse_get(cadena_get, pos, 16);
	loggearInfo(cadena_get);
	free(cadena_get);
	
	muse_free(pos);

	muse_close();

	return EXIT_SUCCESS;

}
