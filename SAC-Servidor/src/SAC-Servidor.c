
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Servidor.h"

GBlock* disco;
size_t tamDisco;
GFile* tablaNodos;
t_bitarray* bitmap;
int socketRespuesta;
int indiceTabla = -1;

typedef enum {
	BORRADO,
	ARCHIVO,
	DIRECTORIO,
} t_estadoNodo;

void renombrar(char* oldpath, char* newpath)
{

	char nombreViejo[MAX_FILENAME_LENGTH];
	char nombreNuevo[MAX_FILENAME_LENGTH];

	strcpy(nombreViejo, nombreObjeto(oldpath) );
	strcpy(nombreNuevo, nombreObjeto(newpath) );

	int indiceViejo = indiceObjeto(nombreViejo);

	if( existeObjeto(newpath) == 1 ) //si el nuevo ya existe, se renombra el viejo y se borra el nuevo
	{

		eliminarObjeto(nombreNuevo);

		memset(tablaNodos[indiceViejo].nombre, 0, MAX_FILENAME_LENGTH);

		strcpy(tablaNodos[indiceViejo].nombre, nombreNuevo);

	}
	else//si el nuevo no existe simplemente se renombra el viejo
	{

		memset(tablaNodos[indiceViejo].nombre, 0, MAX_FILENAME_LENGTH);

		strcpy(tablaNodos[indiceViejo].nombre, nombreNuevo);

	}

}

//recibe un path y devuelve el nombre del objeto o directorio
char* nombreObjeto(char* path)
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
int existeObjeto(char* path)
{
	char* nombre = nombreObjeto(path);

	for (int indActual = 0; indActual <= MAX_FILE_NUMBER; indActual++)
		if (tablaNodos[indActual].estado != 0 && strcmp(nombre,tablaNodos[indActual].nombre) == 0)
			return 1;

	return 0;
}

//recibe un nombre y devuelve el indice de ese objeto en la tabla de nodos
int indiceObjeto(char *nombre)
{

	for (int indActual = 0; indActual < MAX_FILE_NUMBER; indActual++)
		if (tablaNodos[indActual].estado != 0 && strcmp(nombre,tablaNodos[indActual].nombre) == 0)
			return indActual;

	return -1;
}

//devuelve 1 si existe y es un directorio, 0 si no
int esDirectorio(char* nombre)
{
	int indiceDir = indiceObjeto(nombre);
	if(indiceDir>=0 && indiceDir<=MAX_FILE_NUMBER)
	{
		if (tablaNodos[indiceDir].estado == DIRECTORIO)
			return 1;
	}

	return 0;
}

//devuelve 1 si existe y es un archivo, 0 si no
int esArchivo(char* nombre)
{

	int indiceArch = indiceObjeto(nombre);
	if(indiceArch>=0 && indiceArch<=MAX_FILE_NUMBER)
	{
		if (tablaNodos[indiceArch].estado == ARCHIVO)
			return 1;
	}

	return 0;
}

//ve si un directorio esta vacio. Devuelve 1 si esta vacio, 0 si tiene algo
int estaVacio(char* nombre)
{

	int indiceArch = indiceObjeto(nombre);

	for (int indActual = 0; indActual < MAX_FILE_NUMBER; indActual++)
	{
		if(tablaNodos[indActual].estado != 0 && tablaNodos[indActual].padre == indiceArch)
		return 0;
	}
	return 1;
}

//"elimina" un objeto de la tabla
void eliminarObjeto(char* nombre)
{
	int indObjeto = indiceObjeto(nombre);

	//tablaNodos[objeto]->contenido[0] = '\0';
	tablaNodos[indObjeto].estado = 0;
	memset(tablaNodos[indObjeto].nombre, 0, MAX_FILENAME_LENGTH);
	tablaNodos[indObjeto].file_size = 0;
	tablaNodos[indObjeto].padre = 0;
	bitarray_clean_bit(bitmap, ESTRUCTURAS_ADMIN + indObjeto);

}


void agregarObjeto(char* nombre, char* padre, int estado)
{

	int indActual = ESTRUCTURAS_ADMIN + 1;//se saltea los bits de estructuras administrativas y 1 del "/"


	/*while(tablaNodos[indActual].estado != 0 && indActual <MAX_FILE_NUMBER)
		indActual++;*/
	int valorBit = bitarray_test_bit(bitmap, indActual);
	while(valorBit != 0 && indActual <(ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER) )
	{
		indActual++;
		valorBit = bitarray_test_bit(bitmap, indActual);
	}


	GFile* nodoNuevo = tablaNodos + indActual - ESTRUCTURAS_ADMIN;
	bitarray_set_bit(bitmap, indActual);//seteo el bit en el bitmap

	//inicializo valores del nodo
	/*if(estado == ARCHIVO)
		nodoNuevo->bloques_ind[0]*/
	strcpy(nodoNuevo->nombre, nombre);
	nodoNuevo->file_size=0;
	nodoNuevo->estado=estado;

	//linkeo al padre

	if(padre[0] != '\0')
	{
		int indicePadre = indiceObjeto(padre);
		nodoNuevo->padre = indicePadre;
	}
	else
		nodoNuevo->padre = 0;

}

void crearObjeto(char *path, int estado)
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

	agregarObjeto(nombre, padre, estado);

	free(pathCortado);

}


void readdir(char* path)
{

	char* finalizado = malloc(10);
	int enviados = 0;
	int contArray=0;
	char directorio[MAX_FILENAME_LENGTH];
	char* nombreAEnviar;// = malloc(sizeof(MAX_FILENAME_LENGTH)); valgrind
	char** pathCortado = malloc(10*sizeof(char*));

	for(int i=0;i<=9;i++)
		pathCortado[i] = malloc(sizeof(char*));

	pathCortado=string_n_split(path, 10, "/"); // separa el path en strings

	if(pathCortado[contArray]==NULL) // si es el punto de montaje
	{

		for(int indActual = 1; indActual < MAX_FILE_NUMBER; indActual++)
		{
			if(tablaNodos[indActual].estado != 0 && tablaNodos[indActual].padre==0)
			{
				nombreAEnviar = strdup(tablaNodos[indActual].nombre);
				enviarString(socketRespuesta, nombreAEnviar);
				enviados++;

			}
		}

	}
	else	//si no es el punto de montaje
	{
		do
		{
			strcpy(directorio,pathCortado[contArray]);

			contArray++;
		}
		while(pathCortado[contArray]!=NULL);//pone en directorio el ultimo elemento del path

		for (int indActual = 1; tablaNodos[indActual].estado != 0 && indActual < MAX_FILE_NUMBER; indActual++)
		{
			if(tablaNodos[indActual].padre != 0)
			{
				int indicePadre = tablaNodos[indActual].padre;
				if(strcmp(tablaNodos[indicePadre].nombre, directorio)==0)//los hijos del directorio
				{
					nombreAEnviar = strdup(tablaNodos[indActual].nombre);
					enviarString(socketRespuesta, nombreAEnviar);
					enviados++;
				}
			}
		}
	}
	strcpy(finalizado,"dou");//hago esto porque sino enviarString llora
	enviarString(socketRespuesta, finalizado);//"avisa" al cliente que termino

	free(pathCortado);
	//free(nombreAEnviar);
	//free(finalizado);

}


void rutinaServidor(t_mensajeFuse* mensajeRecibido)
{

	char * info = malloc(530);
	int error;

	char* path;// = malloc(500);
	char* contenido= malloc(256);
	char* oldpath = malloc(100);
	char* newpath = malloc(100);
	char* nombreNuevo = malloc(MAX_FILENAME_LENGTH);
	char* nombreViejo = malloc(MAX_FILENAME_LENGTH);



	switch(mensajeRecibido->tipoOperacion)
	{
		case GETATTR:

			recibirString(socketRespuesta, &path);
			if(strcmp(path,"/") == 0)
			{
				enviarInt(socketRespuesta, DIRECTORIO);
				break;
			}
			if(existeObjeto(path) == 1)//si existe envia su estado
			{
				int indice = indiceObjeto(path);
				int estado = tablaNodos[indice].estado;
				enviarInt(socketRespuesta, estado);
			}
			else
				enviarInt(socketRespuesta, BORRADO);
		break;

		case READDIR:

			recibirString(socketRespuesta, &path);

			readdir(path); 	//recibe un path, lo busca en los nodos y devuelve los que cumplen
		break;

		case READ:

			recibirString(socketRespuesta, &path);//recibe el nombre

			int indArch = indiceObjeto(path);

			enviarInt(socketRespuesta, indArch);

			if (indArch == -1)// si no existe
				break;
/*
				for (int indActual = 0; tablaNodos[indActual].estado != 0 && indActual < MAX_FILE_NUMBER; indActual++)
				{
					if( strcmp(tablaNodos[indActual].nombre, path) == 0 )
						strcpy(contenido,tablaNodos[indActual].contenido);	hacer algo con el contenido
				}*/

			strcpy(contenido, "no se, a ver si con esto anda");

			enviarString(socketRespuesta, contenido);

		break;

		case MKDIR:

			recibirString(socketRespuesta, &path);

			crearObjeto(path,DIRECTORIO);
		break;

		case MKNOD:

			recibirString(socketRespuesta, &path);

			crearObjeto(path,ARCHIVO);

		break;

		case WRITE:

		break;

		case RENAME:

			recibirString(socketRespuesta, &path);

			char** pathsRecibidos = string_split(path, ",");//[0] es oldpath, [1] es newpath

			strcpy(oldpath, pathsRecibidos[0]);
			strcpy(newpath, pathsRecibidos[1]);


			nombreNuevo = nombreObjeto(newpath);
			nombreViejo = nombreObjeto(oldpath);


			//ERRORES

			if (esDirectorio(nombreViejo) == 1)
			{

				//si queres renombrar un directorio, el nuevo no tiene que existir o debe estar vacio
				if( (existeObjeto(newpath) == 1) && (estaVacio(nombreNuevo) == 0) )
				{
					enviarInt(socketRespuesta, ENOTEMPTY);//o EEXIST
					break;
				}


				if ( (strcmp(oldpath,"/") == 0) || (strcmp(newpath,"/") == 0) )//si es el punto de montaje tira error
				{
					enviarInt(socketRespuesta, EBUSY);
					break;
				}

				/*ver como hacer esto
				 EINVAL The new pathname contained a path prefix of the old, or, more
				 generally, an attempt was made to make a directory a
				 subdirectory of itself.*/

				if( (existeObjeto(newpath) == 1 ) && (esDirectorio(nombreNuevo) == 0) )//newpath existe pero no es directorio
				{
					enviarInt(socketRespuesta, ENOTDIR);
					break;
				}


			}

			if( (esDirectorio(nombreNuevo) == 1) && (esDirectorio(nombreViejo) == 0) )//new es directorio pero old no
			{
				enviarInt(socketRespuesta, EISDIR);
				break;
			}

			if( existeObjeto(oldpath) == 0 )
			{
				enviarInt(socketRespuesta, ENOENT);
				break;
			}

			//FUNCIONALIDAD

			if(strcmp(oldpath,newpath) == 0)//si son iguales no se hace nada y se devuelve 0
			{
				enviarInt(socketRespuesta, 0);
				break;
			}

			renombrar(oldpath, newpath);

			free(oldpath);
			free(newpath);
			free(nombreNuevo);
			free(nombreViejo);

			enviarInt(socketRespuesta, 0);

		break;

		case UNLINK:

			recibirString(socketRespuesta, &path);//recibe el nombre del directorio

			if(esDirectorio(path) == 1)//si no es directorio tira error
			{
				error = EISDIR;
				enviarInt(socketRespuesta, error);
				break;
			}

			if(esArchivo(path) == 0)//si no esta vacio tira error
			{
				error = ENOENT;
				enviarInt(socketRespuesta, error);
				break;
			}

			eliminarObjeto(path);


			enviarInt(socketRespuesta, 0);//avisa que esta todo piola

		break;

		case RMDIR:

			/*revisar el caso de borrar 2 directorios con la misma id*/


			recibirString(socketRespuesta, &path);//recibe el nombre del directorio


			if (strcmp(path,"/") == 0) //si es el punto de montaje tira error
			{
				error = EBUSY;
				enviarInt(socketRespuesta, error);
				break;
			}

			if(esDirectorio(path) == 0)//si no es directorio tira error
			{
				error = ENOTDIR;
				enviarInt(socketRespuesta, error);
				break;
			}

			if(estaVacio(path) == 0)//si no esta vacio tira error
			{
				error = ENOENT;
				enviarInt(socketRespuesta, error);
				break;
			}


			eliminarObjeto(path);


			enviarInt(socketRespuesta, 0);
		break;

		default:
		break;

	}

	free(info);
	//free(path);
	return;

}

void levantarServidorFUSE()
{

	pthread_t hiloAtendedor = 0;

	int socketServidor = levantarServidor(ip,puerto);

	//char* buffer = malloc(1000);
	char * info = malloc(300);
	char * aux = malloc(50);


	socketRespuesta = (intptr_t) aceptarConexion(socketServidor);

	loggearNuevaConexion(socketRespuesta);

	t_mensajeFuse* mensajeRecibido = malloc(sizeof(t_mensajeFuse) + 500);

	mensajeRecibido = recibirOperacionFuse(socketRespuesta);
	mensajeRecibido->idHilo = hiloAtendedor;

	sprintf(info,"La operacion recibida es %d", mensajeRecibido->tipoOperacion);
	loggearInfo(info);

	while(mensajeRecibido != NULL)
	{

		//if(mensajeRecibido != NULL)
		//{
			int err = pthread_create(&hiloAtendedor, NULL, (void*) rutinaServidor, (void*) mensajeRecibido);

			if(err != 0)
				loggearInfo("Error al crear un nuevo hilo");
			else
			{
				/*sprintf(info,"Se crea el hilo %d",mensajeRecibido->idHilo);
				loggearInfo(info);*/
				pthread_join(hiloAtendedor, NULL);
				hiloAtendedor++;
				mensajeRecibido->idHilo = hiloAtendedor;
			}

		//}
		mensajeRecibido = recibirOperacionFuse(socketRespuesta);

	}

	free(info);
	free(aux);
	free(mensajeRecibido);
	close(socketRespuesta);
}

void levantarConfig()
{
	t_config * unConfig = retornarConfig(pathConfig);

	strcpy(ip,config_get_string_value(unConfig,"IP"));
	strcpy(puerto,config_get_string_value(unConfig,"LISTEN_PORT"));

	config_destroy(unConfig);

	loggearInfoServidor(ip,puerto);

}

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


	/**/


	tamDisco = tamArchivo(argv[1]);
	//size_t tamBitmap = (tamDisco/BLOCK_SIZE/8);

	int discoFD = open(argv[1], O_RDWR,0);

	disco = mmap(NULL, tamDisco, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, discoFD, 0);

	/*			bitmap			*/

	int tamBitmap = tamDisco/BLOCK_SIZE/8; //tamanio del bitmap

	bitmap = bitarray_create_with_mode( (char*) disco + BLOCK_SIZE, tamBitmap, LSB_FIRST);


	//bitarray_set_bit(bitmap, 0);

	/*			tabla de nodos			*/

	disco = disco + 1 + BITMAP_SIZE_IN_BLOCKS; // mueve el puntero por el header y bitmap hasta la tabla de nodos

	tablaNodos = (GFile*) disco;// + 1 + BITMAP_SIZE_IN_BLOCKS;




	loggearInfo("Se inicia el proceso FUSE...");
	levantarConfig();
	levantarServidorFUSE();

	/**/

	disco = disco - 1 - BITMAP_SIZE_IN_BLOCKS;

	msync(disco, tamDisco, MS_SYNC);
	munmap(disco,tamDisco);
	close(discoFD);

	return EXIT_SUCCESS;

}




