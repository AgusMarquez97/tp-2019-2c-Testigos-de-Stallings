/*
 ============================================================================
 Name        : Linuse.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "Linuse.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
	muse_init(getpid(),"127.0.0.1",5003);
	
	muse_map("ejemploArchivo",5,MUSE_MAP_PRIVATE);

	muse_close();


	/*
	uint32_t unaDirec = muse_alloc(100);
	
	char * msj = malloc(100);
	sprintf(msj,"Se recibio la siguiente direccion: %u\n",unaDirec);
	loggearInfo(msj);

	char * cadena_get = malloc(7);
	muse_get(cadena_get,1,7);

	loggearInfo(cadena_get);

	free(cadena_get);

	char * cadena_cpy = "pepe";
	muse_cpy(1,cadena_cpy,strlen(cadena_cpy)+1);

	
	muse_free(unaDirec);

		free(msj);
*/

	return EXIT_SUCCESS;
}
