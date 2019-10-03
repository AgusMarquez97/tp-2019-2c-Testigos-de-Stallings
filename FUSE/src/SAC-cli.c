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

/*NOTA: Por el momento solo puede crear archivos y directorios en el punto de montaje por simplicidad
 * Mas adelante va a tener que ser modificado*/

//arrays que almacenan los directorios, archivos y sus contenidos. A ser cambiado por algo mas lindo mas adelante
char listaDirectorios[256][256];
int indiceDirActual = -1;

char listaArchivos[256][256];
int indiceArchActual = -1;

char listaContenido[256][256];
int indiceContActual = -1;

//agrega el nombre de un directorio a la lista. No el directorio completo
void agregarDirectorio(const char *nombreDir)
{
	indiceDirActual++;
	strcpy(listaDirectorios[indiceDirActual],nombreDir);
}

//devuelve 1 si el directorio existe, 0 si no existe
int existeDirectorio(const char *path)
{
	path++; // Elimina el "/"

	for (int indActual = 0; indActual <= indiceDirActual; indActual++)
		if (strcmp(path,listaDirectorios[indActual]) == 0)
			return 1;

	return 0;
}

//agrega un archivo a la lista y setea su contenido. Ambos indices se tienen que mover juntos
void agregarArchivo(const char *nombre)
{
	indiceArchActual++;
	strcpy(listaArchivos[indiceArchActual], nombre);

	indiceContActual++;
	strcpy(listaContenido[indiceContActual], "");
}

//devuelve 1 si el archivo existe, 0 si no existe
int existeArchivo(const char *path)
{
	path++; // Elimina el "/"

	for (int indActual = 0; indActual <= indiceArchActual; indActual++)
		if (strcmp(path, listaArchivos[indActual]) == 0)
			return 1;

	return 0;
}

//retorna el indice del archivo. -1 si no existe
int indiceArchivo(const char *path)
{
	path++; // Elimina el "/"

	for (int indActual = 0; indActual <= indiceArchActual; indActual++)
		if (strcmp(path,listaArchivos[indActual]) == 0)
			return indActual;

	return -1;
}

//busca el indice del archivo y copia el nuevo contenido. Si no existe el archivo, no hace nada
void escribirEnArchivo(const char *path, const char *contenido)
{
	int indArch = indiceArchivo(path);

	if (indArch == -1)
		return;

	strcpy(listaContenido[indArch], contenido);
}
//va a ser llamada cuando el sistema pida los atributos de un archivo

static int hacer_getattr(const char *path, struct stat *st)
{
	st->st_uid = getuid();		//el duenio del archivo
	st->st_gid = getgid();		//el mismo?
	st->st_atime = time(NULL);	//last "A"ccess time
	st->st_mtime = time(NULL);	//last "M"odification time

	if (strcmp(path, "/") == 0 || existeDirectorio(path) == 1)//si path es el punto de montaje o existe el directorio
	{
		st->st_mode = S_IFDIR | 0755; //bits de permiso
		st->st_nlink = 2;		//num de hardlinks
	}
	else if (existeArchivo(path) == 1)//si es un archivo
	{
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;		//tamanio de los archivos
	}
	else //no existe en el filesystem
	{
		return -ENOENT;
	}
	return 0;
}

//hace una lista de los archivos o directorios disponibles en una ruta específica

static int hacer_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	printf("... Consiguiendo la lista de archivos de %s\n", path);

	filler(buffer, ".", NULL, 0); // llenamos la lista de directorios con el directorio actual
	filler(buffer, "..", NULL, 0); // parent

	if (strcmp(path, "/" ) == 0)
	{
		for (int indActual = 0; indActual <= indiceDirActual; indActual++)
			filler( buffer, listaDirectorios[indActual], NULL, 0);
		for (int indActual = 0; indActual <= indiceArchActual; indActual++)
			filler( buffer, listaArchivos[indActual], NULL, 0);
	}

	return 0;
}

//lee el contenido de un archivo. Retorna el número de bytes que fueron leidos correctamente
static int hacer_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("... Leyendo %s, %u, %u\n", path, offset, size);
	int indArch = indiceArchivo(path);
	if (indArch == -1)
		return -1;
	char *contenido = listaContenido[indArch];
	memcpy(buffer, contenido + offset, size);
	return strlen(contenido) - offset;
}

//va a ser llamada para crear un nuevo directorio. "modo" son los bits de permiso, ver documentacion de mkdir
static int hacer_mkdir(const char *path, mode_t modo)
{
	path++;
	agregarDirectorio(path);
	return 0;
}

//va a ser llamada para crear un nuevo archivo. "dispositivo" tiene que ser especificada si el nuevo archivo
//es un dispositivo, ver documentacion de mknod
static int hacer_mknod(const char *path, mode_t modo, dev_t dispositivo)
{
	path++;
	agregarArchivo(path);
	return 0;
}

//va a ser llamada para escribir contenido en un archivo
static int hacer_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info)
{
	escribirEnArchivo(path, buffer);
	return size;
}

static struct fuse_operations operaciones = {
    .getattr	= hacer_getattr,
    .readdir	= hacer_readdir,
    .read		= hacer_read,
	.mkdir		= hacer_mkdir,
	.mknod		= hacer_mknod,
	.write		= hacer_write,
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
	return fuse_main(argc, argv, &operaciones, NULL);
	//al correrlo pasarle como parametro la direccion donde lo quieras montar
}

void liberarVariablesGlobales()
{
	destruirLog();
}
