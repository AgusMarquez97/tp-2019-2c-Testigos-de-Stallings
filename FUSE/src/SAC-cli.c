/*
 * SAC-cli.c
 *
 *  Created on: 25 sep. 2019
 *      Author: utnso
 */

#define FUSE_USE_VERSION 30

#include <stddef.h>
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include "FUSE.h"

//va a ser llamada cuando el sistema pida los atributos de un archivo

static int hola_getattr( const char *path, struct stat *st )
{
	st->st_uid = getuid();			//el duenio del archivo
	st->st_gid = getgid();			//el mismo?
	st->st_atime = time( NULL );	// last access time
	st->st_mtime = time( NULL );	// last modification time

	if ( strcmp( path, "/" ) == 0 )	//path punto de montaje
	{
		st->st_mode = S_IFDIR | 0755; //0755 son los bits de permiso
		st->st_nlink = 2;
	}
	else
	{
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
	}
	return 0;
}


void levantarServidorFUSE()
{
	int socketRespuesta;
	pthread_t hiloAtendedor = 0;

	int socketServidor = levantarServidor(ip,puerto);

	while(1)
	{
		if((socketRespuesta = (intptr_t) aceptarConexion(socketServidor)) != -1)
		{
			loggearNuevaConexion(socketRespuesta);

			if((hiloAtendedor = makeDetachableThread(rutinaServidor,(void*)(intptr_t)socketRespuesta)) != 0)
			{
			}
			else
			{
				loggearError("Error al crear un nuevo hilo");
			}
		}
	}

}

void rutinaServidor(int socketRespuesta)
{
	loggearInfo(":)");
	sleep(5);

	 // Aca viene la magia...enviar y recibir mierda

}

void levantarConfig()
{
	t_config * unConfig = retornarConfig(pathConfig);

	strcpy(ip,config_get_string_value(unConfig,"IP"));
	strcpy(puerto,config_get_string_value(unConfig,"LISTEN_PORT"));

	config_destroy(unConfig);

	loggearInfoServidor(ip,puerto);
}

int main(void) {
	remove("Linuse.log");
	iniciarLog("FUSE");
	loggearInfo("Se inicia el proceso FUSE...");
	levantarConfig();
	//levantarServidorFUSE();
	liberarVariablesGlobales();
	return EXIT_SUCCESS;
}

void liberarVariablesGlobales()
{
	destruirLog();
}
