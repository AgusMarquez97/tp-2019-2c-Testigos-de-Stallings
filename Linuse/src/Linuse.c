#include "Linuse.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {

	muse_init(getpid(),"127.0.0.1",5003);

	uint32_t pos = muse_alloc(100);
	char* msj = malloc(strlen("Se_recibió_la_siguiente_dirección:_9999999999999999999999"));
	sprintf(msj, "Se recibió la siguiente dirección: %u\n", pos);
	loggearInfo(msj);
	free(msj);

	char* cadena_get = malloc(7);
	muse_get(cadena_get, 1, 7);
	loggearInfo(cadena_get);
	free(cadena_get);

	char* cadena_cpy = "pepe";
	muse_cpy(1, cadena_cpy, strlen(cadena_cpy) + 1);

	muse_map("path_archivo", 10, MUSE_MAP_PRIVATE);

	muse_sync(pos, 10);

	muse_unmap(pos);

	muse_free(pos);

	muse_close();

	return EXIT_SUCCESS;

}
