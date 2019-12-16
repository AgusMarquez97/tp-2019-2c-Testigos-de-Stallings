#define FUSE_USE_VERSION 30

#include <stddef.h>
#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <sys/types.h>
#include <semaphore.h>

#include "FUSE.h"


GBlock* disco;
size_t tamDisco;
GFile* tablaNodos[MAX_FILE_NUMBER];
int indiceTabla=-1;
int32_t socketConexion;
int tamanioTotal = 0;
char* bufferWrite;
size_t offsetWrite = 0;
int contadorWrite = 0;
off_t offsetPrimero = 0;
char* pathWrite;
sem_t mutWrite;

struct t_runtime_options {
	char* disco;
} runtime_options;

#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }

void liberarTabla()
{
	for(int i = 0; i <= indiceTabla; i++)
	{
		free(tablaNodos[i]);
	}
}

//recibe un path y devuelve el nombre del archivo o directorio
char *nombreObjeto(const char *path)
{
	char* nombre = malloc(MAX_FILENAME_LENGTH);
	int contador = 0;
	char** pathCortado = malloc(10*sizeof(char*));

	for(int i=0;i<=9;i++)
		pathCortado[i] = malloc(sizeof(char*));

	pathCortado = string_n_split(path, 10, "/");

	while(pathCortado[contador]!=NULL)
		contador++;

	strcpy(nombre,pathCortado[contador-1] );

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
int indiceObjeto(char *nombre)
{
	if(indiceTabla != -1)
	{
		for (int indActual = 0; indActual <= indiceTabla; indActual++)
			if (strcmp(nombre,tablaNodos[indActual]->nombre) == 0)
				return indActual;
	}

	return -1;
}

//devuelve 1 si existe y es un directorio, 0 si no
int esDirectorio(const char *path)
{
	char* nombre = nombreObjeto(path);
	int indiceDir = indiceObjeto(nombre);
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
	char* nombre = nombreObjeto(path);
	int indiceArch = indiceObjeto(nombre);
	if(indiceArch>=0 && indiceArch<=MAX_FILE_NUMBER)
	{
		if (tablaNodos[indiceArch]->estado == 1)
			return 1;
	}

	return 0;
}

//agrega un directorio a la lista
void agregarDirectorio(char* nombreDir, char* padre)
{
	indiceTabla++;

		tablaNodos[indiceTabla] = (GFile*) malloc(sizeof(GFile));

		//inicializo valores del nodo
		strcpy(tablaNodos[indiceTabla]->nombre,nombreDir);
		tablaNodos[indiceTabla]->file_size=0;
		tablaNodos[indiceTabla]->estado=2;//porque es directorio

		//linkeo al padre
		if(padre[0] != '\0')
		{
			int indicePadre = indiceObjeto(padre);
			tablaNodos[indiceTabla]->padre = tablaNodos[indicePadre];
		}
		else
			tablaNodos[indiceTabla]->padre = NULL;

}

//agrega un archivo a la lista
void agregarArchivo(char *nombreArch, char* padre)
{
	indiceTabla++;

		tablaNodos[indiceTabla] = (GFile*) malloc(sizeof(GFile));
		tablaNodos[indiceTabla]->contenido[0] = '\0';

		//inicializo valores del nodo
		strcpy(tablaNodos[indiceTabla]->nombre,nombreArch);
		tablaNodos[indiceTabla]->file_size=0;
		tablaNodos[indiceTabla]->estado=1;//porque es archivo

		//linkeo al padre
		if(padre[0] != '\0')
		{
			int indicePadre = indiceObjeto(padre);
			tablaNodos[indiceTabla]->padre = tablaNodos[indicePadre];
		}
		else
			tablaNodos[indiceTabla]->padre = NULL;


}



int escribirEnArchivo(void* datosWrite)//(const char *path, const char *contenido)
{
	struct datosWrite* datos = (struct datosWrite*) datosWrite;
	char* path = strdup(datos->path);
	char* buffer = strdup(datos->buffer);
	size_t size = datos->size;
	off_t offset = datos->offset;

	size_t tamBufferLocal;

	if(pathWrite == NULL || strcmp(pathWrite, path) != 0 )
	{
		sem_wait(&mutWrite);
		pathWrite = strdup(path);
	}


	if(bufferWrite == NULL)
	{
		bufferWrite = malloc(size);
		tamBufferLocal = 0;
		offsetPrimero = offset;
	}
	else
		tamBufferLocal = strlen(bufferWrite);

	size_t tamACopiar = strlen(buffer);

	if(buffer[0] == '\0')
		tamACopiar = 1;

	if(size > tamACopiar)
	{
		memcpy(bufferWrite + offsetWrite, buffer, tamACopiar);
		tamBufferLocal = strlen(bufferWrite);
		offsetWrite = offsetWrite + tamACopiar;

	}
	else
	{
		memcpy(bufferWrite + offsetWrite, buffer, size);
		offsetWrite = offsetWrite + size;
		tamACopiar = size;

		enviarInt(socketConexion, WRITE);


		char* nombre = nombreObjeto(path);
		int tamNombre = strlen(nombre) + 1;

		void* bufferEnviar = malloc(sizeof(int) + sizeof(int) + tamNombre + sizeof(size_t) + offsetWrite + sizeof(off_t) );//tamBufferLocal

		int tamEnviar = sizeof(int) + tamNombre + sizeof(size_t) + offsetWrite + sizeof(off_t);

		memcpy(bufferEnviar, &tamEnviar, sizeof(int) );
		memcpy(bufferEnviar + sizeof(int), &tamNombre, sizeof(int) );
		memcpy(bufferEnviar + sizeof(int) + sizeof(int), nombre, tamNombre );
		memcpy(bufferEnviar + sizeof(int) + sizeof(int) + tamNombre, &offsetWrite, sizeof(size_t) );
		memcpy(bufferEnviar + sizeof(int) + sizeof(int) + tamNombre + sizeof(size_t), bufferWrite, offsetWrite);
		memcpy(bufferEnviar + sizeof(int) + sizeof(int) + tamNombre + sizeof(size_t) + offsetWrite, &offsetPrimero, sizeof(off_t) );
		enviar(socketConexion, bufferEnviar, sizeof(int) + tamEnviar );

		free(bufferEnviar);

		free(bufferWrite);
		bufferWrite = NULL;
		offsetWrite = 0;

		free(pathWrite);
		pathWrite = NULL;
		sem_post(&mutWrite);

	}


	return tamACopiar;

}

//ve si un directorio esta vacio. Devuelve 1 si esta vacio, 0 si tiene algo
int estaVacio(char *nombre)
{

	int indiceArch = indiceObjeto(nombre);

	for (int indActual = 0; indActual <= indiceTabla; indActual++)
	{
		if(tablaNodos[indActual]->padre != NULL)
		{
			if(strcmp(tablaNodos[indiceArch]->padre->nombre, nombre)==0)
			return 0;
		}
	}
	return 1;
}

//"elimina" un objeto de la tabla
void eliminarObjeto(char* nombre)
{
	int objeto = indiceObjeto(nombre);

	tablaNodos[objeto]->contenido[0] = '\0';
	tablaNodos[objeto]->estado = 0;
	tablaNodos[objeto]->padre = NULL;
}



//va a ser llamada cuando el sistema pida los atributos de un archivo

static int hacer_getattr(const char *path, struct stat *st)
{


	char* nombre = malloc(MAX_FILENAME_LENGTH);
	int estadoNodo;
	time_t ultimaMod;
	void* bufferDestino = malloc( sizeof(int) + sizeof(time_t) + sizeof(uint32_t) );
	uint32_t tamanio;
	int tamARecibir;

	if(strcmp(path,"/") == 0)
	{
		st->st_uid = getuid();
		st->st_mtime = time(NULL);
		st->st_atime = time(NULL);
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
		return 0;
	}

	nombre = nombreObjeto(path);

	enviarInt(socketConexion, GETATTR);

	enviarString(socketConexion, nombre);

	st->st_uid = getuid();		//el duenio del archivo
	st->st_gid = getgid();
	st->st_mtime = time(NULL);
	st->st_atime = time(NULL);

	recibirInt(socketConexion, &estadoNodo);

	if(estadoNodo != 0)//existe el nodo en el fs
	{
		recibir(socketConexion, &tamARecibir, sizeof(int) );
		recibir(socketConexion, bufferDestino, tamARecibir );
		memcpy(&ultimaMod, bufferDestino, sizeof(time_t) );
		memcpy(&tamanio, bufferDestino + sizeof(time_t), sizeof(uint32_t) );

		st->st_mtime = ultimaMod;
		st->st_atime = ultimaMod;

		if(estadoNodo == DIRECTORIO)
		{
			st->st_mode = S_IFDIR | 0755; //bits de permiso
			st->st_nlink = 2;		//num de hardlinks
			st->st_size = 0;
		}
		else//archivo
		{
			st->st_mode = S_IFREG | 0777;//0644;
			st->st_nlink = 1;
			st->st_size = tamanio;
		}
	}
	else //no existe en el filesystem
		return -ENOENT;

	return 0;

}

//hace una lista de los archivos o directorios disponibles en una ruta específica

static int hacer_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{

	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);


	//enviar READDIR y path a servidor
	enviarInt(socketConexion, READDIR);
	char* pathAEnviar = malloc(300);
	char* nombreRecibido = malloc(MAX_FILENAME_LENGTH);
	int bytesRecibidos;

	strcpy(pathAEnviar,path);
	enviarString(socketConexion, pathAEnviar);

	/*		recibir buffer 		*/
	bytesRecibidos = recibirString(socketConexion, &nombreRecibido);

	while( strcmp(nombreRecibido,"dou") != 0)
	{
		filler(buffer, nombreRecibido, NULL, 0);
		bytesRecibidos = recibirString(socketConexion, &nombreRecibido);
	}

	//free(pathAEnviar);		a valgrind no le gusta
	free(nombreRecibido);

	return 0;

}

//lee el contenido de un archivo. Retorna el número de bytes que fueron leidos correctamente
static int hacer_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{

	int continuar;		//si llega un 0 continua, -1 sale

	char* nombre = nombreObjeto(path);
	int tamNombre = strlen(nombre) + 1;

	void* bufferEnviar = malloc( sizeof(int) + sizeof(int) + tamNombre + sizeof(size_t) + sizeof(off_t) );

	int tamARecibir = sizeof(int) + tamNombre + sizeof(size_t) + sizeof(off_t);

	enviarInt(socketConexion, READ);

	memcpy(bufferEnviar, &tamARecibir, sizeof(int) );
	memcpy(bufferEnviar + sizeof(int), &tamNombre, sizeof(int) );
	memcpy(bufferEnviar + sizeof(int) + sizeof(int), nombre, tamNombre );
	memcpy(bufferEnviar + sizeof(int) + sizeof(int) + tamNombre, &size, sizeof(size_t) );
	memcpy(bufferEnviar + sizeof(int) + sizeof(int) + tamNombre + sizeof(size_t), &offset, sizeof(off_t) );

	enviar(socketConexion, bufferEnviar, sizeof(int) + tamARecibir);

	recibirInt(socketConexion,&continuar);

	if(continuar == -1)//no existe en el FS
		return -1;

	int tamContenido;
	recibirInt(socketConexion, &tamContenido);

	char* contenido = malloc(tamContenido);

	recibir(socketConexion, contenido, tamContenido);

	int tamBuffer;

	if(tamContenido > 0)
	{
		if(size <= tamContenido)
		{
			memcpy(buffer, contenido, size);
			//buffer[size] = '\0';
			tamBuffer = size;
		}
		else
		{
			memcpy(buffer, contenido, tamContenido);
			//buffer[tamContenido] = '\0';
			tamBuffer = tamContenido;
		}

	}
	else
		tamBuffer = tamContenido;

	free(contenido);
	free(bufferEnviar);
	return tamBuffer;
}

//va a ser llamada para crear un nuevo directorio. "modo" son los bits de permiso, ver documentacion de mkdir
static int hacer_mkdir(const char *path, mode_t modo)
{
	char* pathAEnviar = malloc(300);

	enviarInt(socketConexion, MKDIR);

	strcpy(pathAEnviar,path);
	enviarString(socketConexion, pathAEnviar);

	return 0;

}

//va a ser llamada para crear un nuevo archivo. "dispositivo" tiene que ser especificada si el nuevo archivo
//es un dispositivo, ver documentacion de mknod
static int hacer_mknod(const char *path, mode_t modo, dev_t dispositivo)
{

	char* pathAEnviar = malloc(300);

	enviarInt(socketConexion, MKNOD);

	strcpy(pathAEnviar,path);
	enviarString(socketConexion, pathAEnviar);

	return 0;

}

//va a ser llamada para escribir contenido en un archivo
static int hacer_write(const char* path, const char* buffer, size_t size, off_t offset, struct fuse_file_info* info)
{

	pthread_t hiloWrite = 0;
	int tamStruct = strlen(path) + 1 + strlen(buffer) + 1 + sizeof(size_t) + sizeof(off_t);
	struct datosWrite* datos = malloc(tamStruct);

	datos->path = strdup(path);
	datos->buffer = strdup(buffer);
	datos->size = size;
	datos->offset = offset;

	hiloWrite = crearHilo(escribirEnArchivo, (void*) datos);
	if( hiloWrite == 0)
		loggearError("Error al crear un nuevo hilo");

	void* retorno;
	pthread_join(hiloWrite, &retorno);

	free(datos);
	return (int)retorno;

}


//va a ser llamada para remover un directorio
static int hacer_rmdir (const char *path)
{

	enviarInt(socketConexion, RMDIR);

	char* nombre = nombreObjeto(path);

	int error;

	enviarString(socketConexion, nombre);

	/*			*/

	recibirInt(socketConexion, &error);
	if( error != 0 )
	{
		switch(error)
		{
			case EBUSY:
				return EBUSY;
			break;

			case ENOTDIR:
				return ENOTDIR;
			break;

			case ENOENT:
				return ENOENT;
			break;

			default:
			break;
		}
	}


	return 0;
}


//va a ser llamada para remover un archivo
static int hacer_unlink(const char *path)
{

	enviarInt(socketConexion, UNLINK);

	//char* nombre = nombreObjeto(path);

	int error;

	char* pathAEnviar = malloc(strlen(path) + 1);
	strcpy(pathAEnviar, path);

	enviarString(socketConexion, pathAEnviar);

	/*			*/

	recibirInt(socketConexion, &error);

	if( error != 0 )
	{
		switch(error)
		{
			case EISDIR:
				return EISDIR;
			break;

			case ENOENT:
				return ENOENT;
			break;

			default:
			break;
		}
	}


	return 0;


}

//va a ser llamada para renombar un archivo o directorio. Devuelve 0 si funca
int hacer_rename(const char *oldpath, const char *newpath)
{

	int error = 0;

	enviarInt(socketConexion, RENAME);

	int tamOld = strlen(oldpath) + 1;
	int tamNew = strlen(newpath) + 1;
	int tamEnviar = sizeof(int) + tamOld + sizeof(int) + tamNew;

	void* pathsAEnviar = malloc(sizeof(int) + sizeof(int) + tamOld + sizeof(int) + tamNew);

	memcpy(pathsAEnviar, &tamEnviar, sizeof(int) );
	memcpy(pathsAEnviar + sizeof(int), &tamOld, sizeof(int) );
	memcpy(pathsAEnviar + sizeof(int) + sizeof(int), oldpath, tamOld );
	memcpy(pathsAEnviar + sizeof(int) + sizeof(int) + tamOld, &tamNew, sizeof(int) );
	memcpy(pathsAEnviar + sizeof(int) + sizeof(int) + tamOld + sizeof(int), newpath, tamNew);

	enviar(socketConexion, pathsAEnviar, sizeof(int) + tamEnviar );

	free(pathsAEnviar);

	recibirInt(socketConexion, &error);


	return error;

}

int hacer_utimens(const char* path, const struct timespec tv[2])
{
	enviarInt(socketConexion, UTIMENS);
	char* pathAEnviar = strdup(path);

	enviarString(socketConexion, pathAEnviar);
	return 0;

}

int hacer_truncate(const char* path, off_t tamanioNuevo, struct fuse_file_info* fi)
{
	enviarInt(socketConexion, TRUNCATE);

	int resultado;

	int tamanioPath = strlen(path) + 1;
	int tamEnviar = sizeof(int) + tamanioPath + sizeof(off_t);
	void* bufferEnviar = malloc( sizeof(int) + tamEnviar);

	memcpy(bufferEnviar, &tamEnviar, sizeof(int) );
	memcpy(bufferEnviar + sizeof(int), &tamanioPath, sizeof(int) );
	memcpy(bufferEnviar + sizeof(int) + sizeof(int), path, tamanioPath );
	memcpy(bufferEnviar + sizeof(int) + sizeof(int) + tamanioPath, &tamanioNuevo, sizeof(off_t) );
	enviar(socketConexion, bufferEnviar, sizeof(int) + tamEnviar);

	recibirInt(socketConexion,&resultado);

	if(resultado == -1)
		return -1;

	return 0;

}


void levantarClienteFUSE()
{
	socketConexion = levantarCliente(ip,puerto);
}


void levantarConfig()
{
	t_config * unConfig = retornarConfig(pathConfig);

	strcpy(ip,config_get_string_value(unConfig,"IP"));
	strcpy(puerto,config_get_string_value(unConfig,"LISTEN_PORT"));

	config_destroy(unConfig);

	loggearInfoServidor(ip,puerto);

}

static struct fuse_operations operaciones = {


    .getattr	= hacer_getattr,
    .readdir	= hacer_readdir,
	.read		= hacer_read,
	.mkdir		= hacer_mkdir,
	.mknod		= hacer_mknod,
	.write		= hacer_write,
	.rename		= hacer_rename,
	.unlink		= hacer_unlink,
	.rmdir		= hacer_rmdir,
	.utimens	= hacer_utimens,
	.truncate   = hacer_truncate,
};

enum {
	KEY_VERSION,
	//KEY_HELP,
};
/*
static struct fuse_opt fuse_options[] ={
		CUSTOM_FUSE_OPT_KEY("--disk %s", disco, 0),

		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};*/

size_t tamArchivo(char* archivo)
{
	FILE* fd = fopen(archivo, "r");

	fseek(fd, 0L, SEEK_END);
	uint32_t tam = ftell(fd);

	fclose(fd);
	return tam;
}

int main( int argc, char *argv[] )
{

	remove("Linuse.log");
	iniciarLog("FUSE");
	loggearInfo("Se inicia el proceso FUSE...");
	levantarConfig();
	//levantarClienteFUSE();
	sem_init(&mutWrite, 1, 1);
	socketConexion = levantarCliente(ip,puerto);

	int ret = fuse_main(argc, argv, &operaciones, NULL);

	liberarTabla();

	return ret;

}

void liberarVariablesGlobales()
{
	destruirLog();
}
