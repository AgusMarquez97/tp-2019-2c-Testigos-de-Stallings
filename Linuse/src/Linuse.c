#include "Linuse.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {

	muse_init(getpid(),"127.0.0.1",5003);

	size_t tamArchivoMax = 250;
    
	char* homero = malloc(tamArchivoMax);
	char* bart = malloc(tamArchivoMax);
	char* maggie = malloc(tamArchivoMax);
	void* todos = malloc(tamArchivoMax*3);

	uint32_t direccionMemoria1 = muse_map("archivos/homero.txt", tamArchivoMax, MUSE_MAP_SHARED);
	uint32_t direccionMemoria2 = muse_map("archivos/bart.txt", tamArchivoMax, MUSE_MAP_SHARED);
	uint32_t direccionMemoria3 = muse_map("archivos/maggie.txt", tamArchivoMax, MUSE_MAP_SHARED);

	muse_get(homero, direccionMemoria1, tamArchivoMax);
	muse_get(bart, direccionMemoria2, tamArchivoMax);
	muse_get(maggie, direccionMemoria3, tamArchivoMax);

	muse_unmap(direccionMemoria1);
	muse_unmap(direccionMemoria2);
	muse_unmap(direccionMemoria3);
	
	printf("%s\n", homero);
	printf("%s\n", bart);
	printf("%s\n", maggie);

	
	muse_close();

	return EXIT_SUCCESS;

}
