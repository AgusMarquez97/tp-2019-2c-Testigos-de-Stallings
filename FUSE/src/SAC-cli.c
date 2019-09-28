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

static int hacer_getattr( const char *path, struct stat *st )
{
	st->st_uid = getuid();			//el duenio del archivo
	st->st_gid = getgid();			//el mismo?
	st->st_atime = time( NULL );	// last "A"ccess time
	st->st_mtime = time( NULL );	// last "M"odification time

	if ( strcmp( path, "/" ) == 0 )	//si path es el punto de montaje
	{
		st->st_mode = S_IFDIR | 0755; //bits de permiso
		st->st_nlink = 2;		//num de hardlinks
	}
	else
	{
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;		//tamanio de los archivos
	}
	return 0;
}

//hace una lista de los archivos o directorios disponibles en una ruta específica

static int hacer_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{
	printf( "... Consiguiendo la lista de archivos de %s\n", path );

	filler( buffer, ".", NULL, 0 ); // llenamos la lista de directorios con el directorio actual
	filler( buffer, "..", NULL, 0 ); // parent

	if ( strcmp( path, "/" ) == 0 )
	{
		filler( buffer, "archivo1", NULL, 0 );	//agregamos los archivos
		filler( buffer, "archivo2", NULL, 0 );
	}

	return 0;
}

//lee el contenido de un archivo. Retorna el número de bytes que fueron leidos correctamente

static int hacer_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
	printf( "... Leyendo %s, %u, %u\n", path, offset, size );

	char archivo1Text[] = "Archivo 1 presente";
	char archivo2Text[] = "El 2 tambien";
	char *selectedText = NULL;

	if ( strcmp( path, "/archivo1" ) == 0 )		//si es el archivo 1 se asigna su puntero al texto seleccionado
		selectedText = archivo1Text;
	else if ( strcmp( path, "/archivo2" ) == 0 )
		selectedText = archivo2Text;
	else
		return -1;

	memcpy( buffer, selectedText + offset, size );	//copia a buffer el contenido del archivo

	return strlen( selectedText ) - offset;
}

static struct fuse_operations operaciones = {
    .getattr	= hacer_getattr,
    .readdir	= hacer_readdir,
    .read		= hacer_read,
};

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

int main( int argc, char *argv[] )
{
	/*
	remove("Linuse.log");
	iniciarLog("FUSE");
	loggearInfo("Se inicia el proceso FUSE...");
	levantarConfig();
	//levantarServidorFUSE();
	liberarVariablesGlobales();
	return EXIT_SUCCESS;
	*/
	return fuse_main( argc, argv, &operaciones, NULL );
}

void liberarVariablesGlobales()
{
	destruirLog();
}
