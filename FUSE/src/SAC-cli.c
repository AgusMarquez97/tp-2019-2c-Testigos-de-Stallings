#define FUSE_USE_VERSION 30

#include <stddef.h>
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
/*
#include <sys/types.h>


*/
#include "FUSE.h"

GBlock* disco;
size_t tamDisco;
GFile* tablaNodos;//[MAX_FILE_NUMBER];
int indiceTabla=-1;

struct t_runtime_options {
	char* disco;
} runtime_options;

#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }

/*void liberarTabla()
{
	for(int i = 0; i <= indiceTabla; i++)
	{
		free(tablaNodos[i]);
	}
}*/

//recibe un path y devuelve el nombre del archivo o directorio
char *nombreObjeto(const char *path)
{
    char *nombre=NULL;
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
		if (strcmp(nombre,tablaNodos[indActual].nombre) == 0)
			return 1;

	return 0;
}

//retorna el indice del archivo. -1 si no existe
int indiceObjeto(char *nombre)
{
	if(indiceTabla != -1)
	{
		for (int indActual = 0; indActual <= indiceTabla; indActual++)
			if (strcmp(nombre,tablaNodos[indActual].nombre) == 0)
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
		if (tablaNodos[indiceDir].estado == 2)
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
		if (tablaNodos[indiceArch].estado == 1)
			return 1;
	}

	return 0;
}

//agrega un directorio a la lista
void agregarDirectorio(char* nombreDir, char* padre)
{
	indiceTabla++;

	GFile* nodoNuevo = tablaNodos + indiceTabla;

	//inicializo valores del nodo
	strcpy(nodoNuevo->nombre, nombreDir);
	nodoNuevo->file_size=0;
	nodoNuevo->estado=2;


	//linkeo al padre
	if(padre[0] != '\0')
	{
		int indicePadre = indiceObjeto(padre);
		nodoNuevo->padre = tablaNodos + indicePadre;
	}
	else
		nodoNuevo->padre = NULL;

}

//agrega un archivo a la lista
void agregarArchivo(char *nombreArch, char* padre)
{
	indiceTabla++;

	GFile* nodoNuevo = tablaNodos + indiceTabla;



	//inicializo valores del nodo
	nodoNuevo->contenido[0] = '\0';
	strcpy(nodoNuevo->nombre, nombreArch);
	nodoNuevo->file_size=0;
	nodoNuevo->estado=1;

	//linkeo al padre

	if(padre[0] != '\0')
	{
		int indicePadre = indiceObjeto(padre);

		nodoNuevo->padre = tablaNodos + indicePadre;
	}
	else
		nodoNuevo->padre = NULL;


}


//busca el indice del archivo y copia el nuevo contenido. Si no existe el archivo, no hace nada
void escribirEnArchivo(const char *path, const char *contenido)
{
	char* nombre = nombreObjeto(path);

	int indArch = indiceObjeto(nombre);

	if (indArch == -1)
		return;

	strcpy(tablaNodos[indArch].contenido, contenido);
}

//ve si un directorio esta vacio. Devuelve 1 si esta vacio, 0 si tiene algo
int estaVacio(char *nombre)
{

	int indiceArch = indiceObjeto(nombre);

	for (int indActual = 0; indActual <= indiceTabla; indActual++)
	{
		if(tablaNodos[indActual].padre != NULL)
		{
			if(strcmp(tablaNodos[indActual].padre->nombre, nombre)==0)
			return 0;
		}
	}
	return 1;
}

//"elimina" un objeto de la tabla
void eliminarObjeto(char* nombre)
{
	int objeto = indiceObjeto(nombre);

	tablaNodos[objeto].contenido[0] = '\0';
	tablaNodos[objeto].estado = 0;
	tablaNodos[objeto].padre = NULL;
}



//va a ser llamada cuando el sistema pida los atributos de un archivo

static int hacer_getattr(const char *path, struct stat *st)
{

	/*modificar oara archivos borrados*/


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

	int contArray=0;
	char directorio[MAX_FILENAME_LENGTH];
	char** cortado = malloc(10*sizeof(char*));

	for(int i=0;i<=9;i++)
		cortado[i] = malloc(sizeof(char*));

	cortado=string_n_split(path, 10, "/"); // separa el path en strings

	if(cortado[contArray]==NULL) // si es el punto de montaje
	{

		for (int indActual = 0; indActual <= indiceTabla; indActual++)
			if(tablaNodos[indActual].padre==NULL)
				filler(buffer,tablaNodos[indActual].nombre, NULL, 0);
	}
	else	//si no es el punto de montaje
	{
		do
		{
			strcpy(directorio,cortado[contArray]);

			contArray++;
		}
		while(cortado[contArray]!=NULL);

		for (int indActual = 0; indActual <= indiceTabla; indActual++)
		{
			if(tablaNodos[indActual].padre != NULL)
			{
				if(strcmp(tablaNodos[indActual].padre->nombre, directorio)==0)
					filler(buffer,tablaNodos[indActual].nombre, NULL, 0);
			}
		}
	}

	//msync(disco, tamDisco, MS_SYNC);

	free(cortado);

	return 0;
}

//lee el contenido de un archivo. Retorna el número de bytes que fueron leidos correctamente
static int hacer_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("... Leyendo %s, %u, %u\n", path, offset, size);

	char cont[256];

	char* nombre = nombreObjeto(path);

	int indArch = indiceObjeto(nombre);

	if (indArch == -1)// || tablaNodos[indArch]->estado == 2) //si no existe o es un directorio
		return -1;

	strcpy(cont,tablaNodos[indArch].contenido);

	memcpy(buffer, cont + offset, size);

	msync(disco, tamDisco, MS_SYNC);

	return strlen(cont) - offset;


}

//va a ser llamada para crear un nuevo directorio. "modo" son los bits de permiso, ver documentacion de mkdir
static int hacer_mkdir(const char *path, mode_t modo)
{

	char nombre[MAX_FILENAME_LENGTH];
	char padre[MAX_FILENAME_LENGTH];
	int contador = 0;
	char** pathCortado = malloc(10*sizeof(char*));

	for(int i=0;i<=9;i++)
		pathCortado[i] = malloc(sizeof(char*));

	pathCortado = string_n_split(path, 10, "/");

	if(pathCortado[1]==NULL) //estamos en punto de montaje
	{
		strcpy(nombre,pathCortado[0]);
		padre[0]='\0';
	}
	else // "/dir/algo" minimo
	{
		contador++;
		while(pathCortado[contador]!=NULL)
		{
			strcpy(padre,pathCortado[contador-1]);
			strcpy(nombre,pathCortado[contador]);
			contador++;
		}
	}

	agregarDirectorio(nombre,padre);

	msync(disco, tamDisco, MS_SYNC);

	free(pathCortado);

	return 0;

}

//va a ser llamada para crear un nuevo archivo. "dispositivo" tiene que ser especificada si el nuevo archivo
//es un dispositivo, ver documentacion de mknod
static int hacer_mknod(const char *path, mode_t modo, dev_t dispositivo)
{

	char nombre[MAX_FILENAME_LENGTH];
	char padre[MAX_FILENAME_LENGTH];
	int contador = 0;
	char** pathCortado = malloc(10*sizeof(char*));

		for(int i=0;i<=9;i++)
			pathCortado[i] = malloc(sizeof(char*));

	pathCortado = string_n_split(path, 10, "/");

	if(pathCortado[1]==NULL)
	{
		strcpy(nombre,pathCortado[0]);
		padre[0]='\0';
	}
	else
	{
		contador++;
		while(pathCortado[contador]!=NULL)
		{
			strcpy(padre,pathCortado[contador-1]);
			strcpy(nombre,pathCortado[contador]);
			contador++;
		}
	}

	agregarArchivo(nombre,padre);

	msync(disco, tamDisco, MS_SYNC);

	free(pathCortado);

	return 0;

}

//va a ser llamada para escribir contenido en un archivo
static int hacer_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info)
{
	escribirEnArchivo(path, buffer);
	return size;
}


//va a ser llamada para remover un directorio
static int hacer_rmdir (const char *path)
{

	/*revisar que no deja volver a crear dir con mismo nombre despues de borrar
	 * aparece en el disco pero no en mount point*/

	char* nombre = nombreObjeto(path);

	if (strcmp(path,"/") == 0) //si es el punto de montaje tira error
		return EBUSY;

	if(esDirectorio(path) == 0)//si no es directorio tira error
		return ENOTDIR;

	if(estaVacio(nombre) == 0)//si no esta vacio tira error
		return ENOENT;

	eliminarObjeto(nombre);

	return 0;
}


//va a ser llamada para remover un archivo
static int hacer_unlink (const char *path)
{
	if(esDirectorio(path) == 1)
		return EISDIR;

	if(esArchivo(path) == 0)
		return ENOENT;

	char* nombre = nombreObjeto(path);

	eliminarObjeto(nombre);

	return 0;
}


void levantarClienteFUSE()
{

	//pthread_t hiloAtendedor = 0;

	int32_t cliente = levantarCliente(ip,puerto);

	while(1)
	{
		char mensaje[1000];
		scanf("%s", mensaje);
		//send(cliente,mensaje,strlen(mensaje),0);
		int bytesEnviados = enviar(cliente, mensaje, strlen(mensaje));
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

static struct fuse_operations operaciones = {


	.unlink		= hacer_unlink,
	.rmdir		= hacer_rmdir,
    .getattr	= hacer_getattr,
    .readdir	= hacer_readdir,
	.read		= hacer_read,
	.mkdir		= hacer_mkdir,
	.mknod		= hacer_mknod,
	.write		= hacer_write,
};

enum {
	KEY_VERSION,
	//KEY_HELP,
};

static struct fuse_opt fuse_options[] ={
		CUSTOM_FUSE_OPT_KEY("--disk %s", disco, 0),

		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};

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

	//descomentar y comentar todo lo demas para que solo reciba el punto de montaje

	/*int ret = fuse_main(argc, argv, &operaciones, NULL);

	liberarTabla();

	return ret;*/

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);


	memset(&runtime_options, 0, sizeof(struct t_runtime_options));


	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1){

		perror("Argumentos invalidos!");
		return EXIT_FAILURE;
	}


	if( runtime_options.disco != NULL )
	{
		printf("%s\n", runtime_options.disco);
	}

	tamDisco= tamArchivo(runtime_options.disco);

	int discoFD = open(runtime_options.disco, O_RDWR,0);

	disco = mmap(NULL, tamDisco, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, discoFD, 0);

	disco = disco + 1 + BITMAP_SIZE_IN_BLOCKS; // mueve el puntero dos posiciones hasta la tabla de nodos

	tablaNodos = (GFile*) disco;

	int ret = fuse_main(args.argc, args.argv, &operaciones, NULL);

	munmap(disco,tamDisco);
	close(discoFD);

	//liberarTabla();

	return ret;

	/*remove("Linuse.log");
	iniciarLog("FUSE");
	loggearInfo("Se inicia el proceso FUSE...");
	levantarConfig();
	levantarClienteFUSE();

	return EXIT_SUCCESS;*/

}

void liberarVariablesGlobales()
{
	destruirLog();
}
