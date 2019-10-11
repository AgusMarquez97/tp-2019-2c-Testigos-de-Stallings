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


GFile *tablaNodos[5];//hacer hasta MAX_FILE_NUMBER
int indiceTabla=-1;

void inicializar()
{
	for(int i = 0; i <= 5; i++)
	{
		tablaNodos[i]=malloc(sizeof(GFile));
		tablaNodos[i]->contenido=malloc(sizeof(char*));
		tablaNodos[i]->nombre=malloc(sizeof(char*));
	}
}

void liberarTabla()
{
	for(int i = 0; i <= 5; i++)
	{

		free(tablaNodos[i]->nombre);
		free(tablaNodos[i]->contenido);
		free(tablaNodos[i]);
	}
}

//recibe un path y devuelve el nombre del archivo o directorio
char *nombreObjeto(const char *path)
{
    char *nombre;
	char *reverso =	string_reverse(path);
    int cont=0;
    while(reverso[cont]!='/')
    {
        cont++;
    }
    reverso[cont]='\0';
    nombre=string_reverse(reverso);
    return nombre;
}

//devuelve 1 si el directorio o archivo existe, 0 si no existe
int existeObjeto(const char *path)
{
	char *nombre = nombreObjeto(path);

	for (int indActual = 0; indActual <= indiceTabla; indActual++)
		if (strcmp(nombre,tablaNodos[indActual]->nombre) == 0)
			return 1;

	return 0;
}

//retorna el indice del archivo. -1 si no existe
int indiceObjeto(const char *path)
{
	char *nombre=nombreObjeto(path);

	for (int indActual = 0; indActual <= indiceTabla; indActual++)
		if (strcmp(nombre,tablaNodos[indActual]->nombre) == 0)
			return indActual;

	return -1;
}

//devuelve 1 si existe y es un directorio, 0 si no
int esDirectorio(const char *path)
{
	int indiceDir = indiceObjeto(path);
	if(indiceDir>=0 && indiceDir<=MAX_FILE_NUMBER)
	{
		if (tablaNodos[indiceDir]->estado == 2)
			return 1;
	}

	return 0;
}

//devuelve 1 si existe y es un archivo, 0 si no
int esArchivo(const char *path)
{
//if existeobjeto(path)
	int indiceArch = indiceObjeto(path);
	if(indiceArch>=0 && indiceArch<=MAX_FILE_NUMBER)
	{
		if (tablaNodos[indiceArch]->estado == 1)
			return 1;
	}

	return 0;
}

//agrega un directorio a la lista
void agregarDirectorio(char *nombreDir)
{
	indiceTabla++;

	strcpy(tablaNodos[indiceTabla]->nombre,nombreDir);
	tablaNodos[indiceTabla]->file_size=0;
	tablaNodos[indiceTabla]->estado=2;//porque es directorio
}

//agrega un archivo a la lista
void agregarArchivo(char *nombreDir)
{
	indiceTabla++;

	strcpy(tablaNodos[indiceTabla]->nombre,nombreDir);

	tablaNodos[indiceTabla]->file_size=0;
	tablaNodos[indiceTabla]->estado=1;//porque es archivo

}


//busca el indice del archivo y copia el nuevo contenido. Si no existe el archivo, no hace nada
void escribirEnArchivo(const char *path, const char *contenido)
{
	int indArch = indiceObjeto(path);

	if (indArch == -1)
		return;

	strcpy(tablaNodos[indArch]->contenido, contenido);
}



//va a ser llamada cuando el sistema pida los atributos de un archivo

static int hacer_getattr(const char *path, struct stat *st)
{
	printf("pidiendo atributos");
	st->st_uid = getuid();		//el duenio del archivo
	st->st_gid = getgid();		//el mismo?
	st->st_atime = time(NULL);	//last "A"ccess time
	st->st_mtime = time(NULL);	//last "M"odification time

	if(strcmp(path,"/") == 0 || esDirectorio(path) == 1)
	{
		st->st_mode = S_IFDIR | 0755; //bits de permiso
		st->st_nlink = 2;		//num de hardlinks
	}
	else if (esArchivo(path) == 1)//si es un archivo
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

	filler(buffer, ".", NULL, 0); // llenamos la lista de directorios con el directorio actual
	filler(buffer, "..", NULL, 0); // padre

	if (strcmp(path, "/" ) == 0)//ls del punto de montaje
	{
		for (int indActual = 0; indActual <= indiceTabla; indActual++)
			filler(buffer,tablaNodos[indActual]->nombre, NULL, 0);
	}
	return 0;

}

//lee el contenido de un archivo. Retorna el número de bytes que fueron leidos correctamente
static int hacer_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("... Leyendo %s, %u, %u\n", path, offset, size);


	int indArch = indiceObjeto(path);

	if (indArch == -1)// || tablaNodos[indArch]->estado == 2) //si no existe o es un directorio
		return -1;

	char *cont= tablaNodos[indArch]->contenido;


	memcpy(buffer, cont + offset, size);
	return strlen(cont) - offset;


}

//va a ser llamada para crear un nuevo directorio. "modo" son los bits de permiso, ver documentacion de mkdir
static int hacer_mkdir(const char *path, mode_t modo)
{
	char* nombre = nombreObjeto(path);

	agregarDirectorio(nombre);

	return 0;

}

//va a ser llamada para crear un nuevo archivo. "dispositivo" tiene que ser especificada si el nuevo archivo
//es un dispositivo, ver documentacion de mknod
static int hacer_mknod(const char *path, mode_t modo, dev_t dispositivo)
{
	char* nombre = nombreObjeto(path);

	agregarArchivo(nombre);

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

	return EXIT_SUCCESS;
*/

	inicializar();

	int ret = fuse_main(argc, argv, &operaciones, NULL);

	liberarTabla();

	return ret;
}

void liberarVariablesGlobales()
{
	destruirLog();
}
