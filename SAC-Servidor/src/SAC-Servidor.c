
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

#include "Servidor.h"

GBlock* disco;
size_t tamDisco;
GFile* tablaNodos;
t_bitarray* bitmap;
//int socketRespuesta;
int cantBloqueDatos;
sem_t mutWrite;
sem_t mutRename;
int tamBitmap;

void escribir(int indArchivo, char* contenido, size_t tamanio)
{
	//anda con poco contenido

	int offset = 0;
	int bloqInd = 0;
	int bloqDatos = 0;

	sem_wait(&mutWrite);

	if(tamanio < BLOCK_SIZE)
		strcpy(tablaNodos[indArchivo].bloques_ind[0]->bloquesDatos[0]->bytes, contenido);
	else
	{
		while(offset < tamanio && bloqInd < BLOQUES_INDIRECTOS)
		{
			while( offset < tamanio && bloqDatos < BLOQUES_DATOS)
			{
				strcpy(tablaNodos[indArchivo].bloques_ind[bloqInd]->bloquesDatos[bloqDatos]->bytes, contenido + offset);
				offset = offset + BLOCK_SIZE;
			}
		}
	}

	tablaNodos[indArchivo].file_size = tamanio;

	sem_post(&mutWrite);

}

void setearTiempo(char* path)
{
	char* nombre = nombreObjeto(path);
	int indice = indiceObjeto(nombre);

	tablaNodos[indice].fecha_creacion = time(NULL);
	tablaNodos[indice].fecha_modif = time(NULL);
}

void renombrar(char* oldpath, char* newpath)
{

	char nombreViejo[MAX_FILENAME_LENGTH];
	char nombreNuevo[MAX_FILENAME_LENGTH];

	sem_wait(&mutRename);

	strcpy(nombreViejo, nombreObjeto(oldpath) );
	strcpy(nombreNuevo, nombreObjeto(newpath) );

	int indiceViejo = indiceObjeto(nombreViejo);

	if( existeObjeto(nombreNuevo) == 1 ) //si el nuevo ya existe, se renombra el viejo y se borra el nuevo
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

	sem_post(&mutRename);
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
int existeObjeto(char* nombre)
{
	//char* nombre = nombreObjeto(path);
	if (nombre[0] == '.')
		return 0;
	if(strncmp(nombre, "autorun",7) == 0)
		return 0;
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


	int cont = 0;
	int contDatos = 0;

	if(tablaNodos[indObjeto].estado == ARCHIVO)
	{
		while(tablaNodos[indObjeto].bloques_ind[cont] != NULL && cont < BLOQUES_INDIRECTOS)
		{

			while(tablaNodos[indObjeto].bloques_ind[cont]->bloquesDatos[contDatos] != NULL && contDatos < BLOQUES_DATOS)
			{

				if(tablaNodos[indObjeto].bloques_ind[cont]->bloquesDatos[contDatos]->bytes[0] != 0)
				{
					memset(tablaNodos[indObjeto].bloques_ind[cont]->bloquesDatos[contDatos]->bytes, 0, BLOCK_SIZE);
				}
				contDatos++;
			}
			(tablaNodos[indObjeto].bloques_ind[cont]->bloquesDatos[contDatos]) = NULL;
			cont++;
		}
		(tablaNodos[indObjeto].bloques_ind[cont]) = NULL;
		memset(tablaNodos[indObjeto].bloques_ind, 0 , BLOQUES_INDIRECTOS);
	}

	//recorre el bitmap y pone en 0 los bloques que estan vacios
	for(int indiceBloque = ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER; indiceBloque < (ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER + cantBloqueDatos); indiceBloque++)
	{
		int valorBit = bitarray_test_bit(bitmap, indiceBloque);
		if( valorBit == 1 && (tablaNodos + indiceBloque - ESTRUCTURAS_ADMIN) == NULL)
			bitarray_clean_bit(bitmap, indiceBloque);
	}

	tablaNodos[indObjeto].estado = 0;
	memset(tablaNodos[indObjeto].nombre, 0, MAX_FILENAME_LENGTH);
	tablaNodos[indObjeto].file_size = 0;
	tablaNodos[indObjeto].padre = 0;
	bitarray_clean_bit(bitmap, ESTRUCTURAS_ADMIN + indObjeto);

}


void agregarObjeto(char* nombre, char* padre, int estado)
{

	int indActual = ESTRUCTURAS_ADMIN + 1; //se saltea los bits de estructuras administrativas y la raiz


	int valorBit = bitarray_test_bit(bitmap, indActual);
	while(valorBit != 0 && indActual < (ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER) )
	{
		indActual++;
		valorBit = bitarray_test_bit(bitmap, indActual);
	}


	GFile* nodoNuevo = tablaNodos + indActual - ESTRUCTURAS_ADMIN;
	bitarray_set_bit(bitmap, indActual);//seteo el bit en el bitmap

	nodoNuevo->file_size=0;

	//inicializo valores del nodo
	if(estado == ARCHIVO)
	{
		//bloque de punteros indirectos

		int indBloque = ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER;//el indice en el bitmap arranca despues de eso
		valorBit = bitarray_test_bit(bitmap, indBloque);
		while(valorBit != 0 && indBloque < (ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER + cantBloqueDatos) )
		{
			indBloque++;
			valorBit = bitarray_test_bit(bitmap, indBloque);
		}
		bitarray_set_bit(bitmap, indBloque);
		valorBit = bitarray_test_bit(bitmap, indBloque);

		nodoNuevo->bloques_ind[0] = (IndBlock*) (tablaNodos + indBloque - ESTRUCTURAS_ADMIN);

		for(int i = 1; i < BLOQUES_INDIRECTOS; i++)
		{
			nodoNuevo->bloques_ind[i] = NULL;
		}

		//bloque de datos


		while(valorBit != 0 && indBloque < (ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER + cantBloqueDatos) )
		{
			indBloque++;
			valorBit = bitarray_test_bit(bitmap, indBloque);
		}
		bitarray_set_bit(bitmap, indBloque);

		for(int i = 1; i < BLOQUES_DATOS; i++)
		{
			nodoNuevo->bloques_ind[0]->bloquesDatos[i] = NULL;
		}
		nodoNuevo->bloques_ind[0]->bloquesDatos[0] = (GBlock*) (tablaNodos + indBloque - ESTRUCTURAS_ADMIN);

	}

	strcpy(nodoNuevo->nombre, nombre);

	nodoNuevo->estado=estado;

	time_t tiempo = time(NULL);

	nodoNuevo->fecha_creacion = tiempo;
	nodoNuevo->fecha_modif = tiempo;

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


void readdir(char* path, int socketRespuesta)
{

	char* finalizado = malloc(10);
	int enviados = 0;
	int contArray=0;
	char directorio[MAX_FILENAME_LENGTH];
	char* nombreAEnviar;
	char** pathCortado = malloc(10*sizeof(char*));

	for(int i=0;i<=9;i++)
		pathCortado[i] = malloc(sizeof(char*));

	pathCortado=string_n_split(path, 10, "/"); // separa el path en strings


	if(pathCortado[contArray]==NULL) // si es el punto de montaje
	{

		for(int indActual = 0; indActual < MAX_FILE_NUMBER; indActual++)
		{
			if(bitarray_test_bit(bitmap, indActual + ESTRUCTURAS_ADMIN) != 0)
			{
				if(tablaNodos[indActual].estado != 0 && tablaNodos[indActual].padre==0)
				{
					nombreAEnviar = strdup(tablaNodos[indActual].nombre);
					enviarString(socketRespuesta, nombreAEnviar);
					enviados++;
				}
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

		for (int indActual = 0; indActual < MAX_FILE_NUMBER; indActual++)//tablaNodos[indActual].estado != 0 &&
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


void rutinaServidor(t_mensajeFuse* mensajeRecibido, int socketRespuesta)
{

	switch(mensajeRecibido->tipoOperacion)
	{
		case GETATTR:
		{//hago los cases con llaves para declarar variables sin problemas

			char* pathGetattr = malloc(200);
			recibirString(socketRespuesta, &pathGetattr);//recibe nombre

			if(existeObjeto(pathGetattr) == 1)//si existe envia su estado y ultima modificacion
			{

				int indice = indiceObjeto(pathGetattr);
				uint8_t estado = tablaNodos[indice].estado;
				time_t ultimaMod = tablaNodos[indice].fecha_modif;
				void* buffer = malloc( sizeof(time_t) );

				enviarInt(socketRespuesta, estado);

				memcpy(buffer, &ultimaMod, sizeof(time_t) );
				enviarVoid(socketRespuesta, buffer, sizeof(buffer) );

				free(buffer);

			}
			else
			{
				enviarInt(socketRespuesta, BORRADO);
			}

			free(pathGetattr);

			break;
		}

		case READDIR:
		{
			char* pathReaddir = malloc(200);
			recibirString(socketRespuesta, &pathReaddir);

			readdir(pathReaddir, socketRespuesta); 	//recibe un path, lo busca en los nodos y devuelve los que cumplen

			free(pathReaddir);

			break;
		}
		case READ:
		{
			int tamNombre;
			size_t size;
			off_t offset;
			char* nombreRead = malloc(MAX_FILENAME_LENGTH);
			int tamBuffer;
			int indArch;

			recibir(socketRespuesta, &tamBuffer, sizeof(int) );

			void* buffer = malloc(tamBuffer);

			recibir(socketRespuesta, buffer, tamBuffer);

			memcpy(&tamNombre, buffer, sizeof(int) );
			memcpy(nombreRead, buffer + sizeof(int), tamNombre );
			memcpy(&size, buffer + sizeof(int) + tamNombre , sizeof(size_t) );
			memcpy(&offset, buffer + sizeof(int) + tamNombre + sizeof(size_t), sizeof(off_t) );

			void* enviar = malloc(size + offset);
			indArch = indiceObjeto(nombreRead);

			enviarInt(socketRespuesta, indArch);

			if (indArch == -1)// si no existe
				break;

			int cont = 0;
			int contDatos = 0;
			char* contenido = malloc(offset + size);

			while(tablaNodos[indArch].bloques_ind[cont] != NULL && cont < BLOQUES_INDIRECTOS)
			{

				while(tablaNodos[indArch].bloques_ind[cont]->bloquesDatos[contDatos] != NULL && contDatos < BLOQUES_DATOS)
				{

					if(tablaNodos[indArch].bloques_ind[cont]->bloquesDatos[contDatos]->bytes[0] != 0)
					{

						strcpy(contenido,tablaNodos[indArch].bloques_ind[cont]->bloquesDatos[contDatos]->bytes);
						//size = size - ( strlen(contenido1) );

					}
					contDatos++;

				}

				cont++;
			}


			//strcpy(contenido1,tablaNodos[indArch].bloques_ind[0]->bloquesDatos[0]->bytes);

			memcpy(enviar, contenido + offset, size);

			enviarString(socketRespuesta, enviar);

			free(nombreRead);
			free(buffer);
			//free(enviar);
			free(contenido);

			break;
		}
		case MKDIR:
		{
			char* pathMkdir = malloc(200);
			recibirString(socketRespuesta, &pathMkdir);

			crearObjeto(pathMkdir,DIRECTORIO);

			free(pathMkdir);

			break;
		}
		case MKNOD:
		{
			char* pathMknod = malloc(200);
			recibirString(socketRespuesta, &pathMknod);

			crearObjeto(pathMknod,ARCHIVO);

			free(pathMknod);

			break;
		}
		case WRITE:
		{

			int indArch;
			int tamARecibir;
			recibir(socketRespuesta,&tamARecibir, sizeof(int));

			void* bufferWrite = malloc(tamARecibir);
			recibir(socketRespuesta, bufferWrite, tamARecibir);

			char* nombre = malloc(MAX_FILENAME_LENGTH);
			int tamNom;
			size_t tamContenido;

			memcpy(&tamNom, bufferWrite, sizeof(int));
			memcpy(nombre, (char*) (bufferWrite + sizeof(int) ), tamNom);
			memcpy(&tamContenido,bufferWrite + sizeof(int) + tamNom, sizeof(size_t) );

			char* contenido = malloc(tamContenido);
			memcpy(contenido, (char*) (bufferWrite + sizeof(int) + tamNom + sizeof(size_t) ), tamContenido);

			indArch = indiceObjeto(nombre);
			if (indArch == -1)
				break;

			escribir(indArch, contenido, tamContenido);

			free(contenido);
			free(nombre);
			free(bufferWrite);

			break;
		}

		case RENAME:
		{
			char* oldpath = malloc(100);
			char* newpath = malloc(100);
			char* nombreNuevo = malloc(MAX_FILENAME_LENGTH);
			char* nombreViejo = malloc(MAX_FILENAME_LENGTH);

			char* pathRename = malloc(200);
			recibirString(socketRespuesta, &pathRename);

			char** pathsRecibidos = string_split(pathRename, ",");//[0] es oldpath, [1] es newpath

			strcpy(oldpath, pathsRecibidos[0]);
			strcpy(newpath, pathsRecibidos[1]);


			nombreNuevo = nombreObjeto(newpath);
			nombreViejo = nombreObjeto(oldpath);


			//ERRORES

			if (esDirectorio(nombreViejo) == 1)
			{

				//si queres renombrar un directorio, el nuevo no tiene que existir o debe estar vacio
				if( (existeObjeto(nombreNuevo) == 1) && (estaVacio(nombreNuevo) == 0) )
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

				if( (existeObjeto(nombreNuevo) == 1 ) && (esDirectorio(nombreNuevo) == 0) )//newpath existe pero no es directorio
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

			if( existeObjeto(nombreViejo) == 0 )
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

			enviarInt(socketRespuesta, 0);

			free(oldpath);
			free(newpath);
			free(nombreNuevo);
			free(nombreViejo);
			free(pathRename);

			break;
		}
		case UNLINK:
		{
			int error;
			char* nombreUnlink = malloc(200);
			recibirString(socketRespuesta, &nombreUnlink);//recibe el nombre del directorio

			if(esDirectorio(nombreUnlink) == 1)//si no es directorio tira error
			{
				error = EISDIR;
				enviarInt(socketRespuesta, error);
				break;
			}

			if(esArchivo(nombreUnlink) == 0)//si no esta vacio tira error
			{
				error = ENOENT;
				enviarInt(socketRespuesta, error);
				break;
			}

			eliminarObjeto(nombreUnlink);


			enviarInt(socketRespuesta, 0);//avisa que esta todo piola
			free(nombreUnlink);

			break;
		}
		case RMDIR:
		{

			/*revisar el caso de borrar 2 directorios con la misma id*/
			int error;
			char* nombreRmdir = malloc(200);
			recibirString(socketRespuesta, &nombreRmdir);//recibe el nombre del directorio


			if (strcmp(nombreRmdir,"/") == 0) //si es el punto de montaje tira error
			{
				error = EBUSY;
				enviarInt(socketRespuesta, error);
				break;
			}

			if(esDirectorio(nombreRmdir) == 0)//si no es directorio tira error
			{
				error = ENOTDIR;
				enviarInt(socketRespuesta, error);
				break;
			}

			if(estaVacio(nombreRmdir) == 0)//si no esta vacio tira error
			{
				error = ENOENT;
				enviarInt(socketRespuesta, error);
				break;
			}


			eliminarObjeto(nombreRmdir);


			enviarInt(socketRespuesta, 0);

			free(nombreRmdir);

			break;
		}
		case UTIMENS:
		{
			char* pathUtimens = malloc(200);
			recibirString(socketRespuesta, &pathUtimens);

			setearTiempo(pathUtimens);

			free(pathUtimens);

			break;
		}
		default:
		break;

	}

	return;

}

void levantarServidorFUSE()
{

	pthread_t hiloAtendedor = 0;
	int socketRespuesta;
	int socketServidor = levantarServidor(ip,puerto);

	sem_init(&mutWrite, 0, 1);
	sem_init(&mutRename, 0, 1);

	while(1)
	{
		if( (socketRespuesta = (intptr_t)aceptarConexion(socketServidor) ) != -1)
		{
			loggearNuevaConexion(socketRespuesta);
			int *p_socket = malloc(sizeof(int));
			*p_socket = socketRespuesta;

			if((hiloAtendedor = makeDetachableThread(recibirOperaciones,(void*)p_socket)) == 0)
				loggearError("Error al crear un nuevo hilo");
		}
	}

	/*socketRespuesta = (intptr_t) aceptarConexion(socketServidor);

	loggearNuevaConexion(socketRespuesta);*/

	close(socketRespuesta);
}




void recibirOperaciones(int* p_socket)
{

	int socketRespuesta = *p_socket;

	pthread_t hiloAtendedor = 0;

	t_mensajeFuse* mensajeRecibido = malloc(sizeof(t_mensajeFuse) + 500);

	mensajeRecibido = recibirOperacionFuse(socketRespuesta);
	mensajeRecibido->idHilo = hiloAtendedor;



	while(mensajeRecibido != NULL)
	{

			rutinaServidor(mensajeRecibido, socketRespuesta);
		/*int err = pthread_create(&hiloAtendedor, NULL, (void*) rutinaServidor, (void*) mensajeRecibido);

			if(err != 0)
				loggearInfo("Error al crear un nuevo hilo");
			else
			{
				pthread_join(hiloAtendedor, NULL);
				hiloAtendedor++;
				mensajeRecibido->idHilo = hiloAtendedor;
			}*/

		mensajeRecibido = recibirOperacionFuse(socketRespuesta);

	}

	free(mensajeRecibido);

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

	tamBitmap = tamDisco/BLOCK_SIZE/8; //tamanio del bitmap

	bitmap = bitarray_create_with_mode( (char*) disco + BLOCK_SIZE, tamBitmap, LSB_FIRST);


	//bitarray_set_bit(bitmap, 0);

	/*			tabla de nodos			*/

	disco = disco + 1 + BITMAP_SIZE_IN_BLOCKS; // mueve el puntero por el header y bitmap hasta la tabla de nodos

	tablaNodos = (GFile*) disco;// + 1 + BITMAP_SIZE_IN_BLOCKS;

	cantBloqueDatos = (tamDisco/BLOCK_SIZE) - 1 - (tamBitmap/BLOCK_SIZE) - 1024;

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




