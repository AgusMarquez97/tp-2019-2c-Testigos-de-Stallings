
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
int cantBloqueDatos;
sem_t mutWrite;
sem_t mutRename;
int tamBitmap;
int tamanioTotal = 0;
off_t offsetWrite = 0;

void padreEHijo(char* path, char* nombre, char* padre)
{
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
}

void rellenarArchivo(int indiceArch, int tamanioNuevo)
{
	int offset = tablaNodos[indiceArch].file_size;
	int cantidadPadding = tamanioNuevo - offset;
	int bloqInd = 0;
	int bloqDatos = 0;

	if(offset == 0 && tamanioNuevo > 0)//si es un archivo vacio que queres agrandar le creas un bloque de cada tipo
	{
		//bloque de punteros indirectos

		int indBloque = ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER;//el indice en el bitmap arranca despues de eso
		int valorBit = bitarray_test_bit(bitmap, indBloque);
		while(valorBit != 0 && indBloque < (ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER + cantBloqueDatos) )
		{
			indBloque++;
			valorBit = bitarray_test_bit(bitmap, indBloque);
		}
		bitarray_set_bit(bitmap, indBloque);
		valorBit = bitarray_test_bit(bitmap, indBloque);

		tablaNodos[indiceArch].bloques_ind[0] =  (IndBlock*)tablaNodos + indBloque - ESTRUCTURAS_ADMIN;

		loggearInfo("Puntero a bloque indirecto seteado");
		loggearInfo("Bit seteado");

		//bloque de datos
		while(valorBit != 0 && indBloque < (ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER + cantBloqueDatos) )
		{
			indBloque++;
			valorBit = bitarray_test_bit(bitmap, indBloque);
		}
		bitarray_set_bit(bitmap, indBloque);

		tablaNodos[indiceArch].bloques_ind[0]->bloquesDatos[0] = (GBlock*)tablaNodos + indBloque - ESTRUCTURAS_ADMIN;

		loggearInfo("Puntero a bloque de datos seteado");
		loggearInfo("Bit seteado");
	}

	if(tamanioNuevo < BLOCK_SIZE && offset != 0)//en este caso tam actual siempre va a ser < block size
		memset(tablaNodos[indiceArch].bloques_ind[0]->bloquesDatos[0]->bytes + offset , '@', cantidadPadding);
	else
	{
		while(cantidadPadding > 0 && bloqInd < BLOQUES_INDIRECTOS)
		{

			while(cantidadPadding > 0 && bloqDatos < BLOQUES_DATOS)
			{

				memset(tablaNodos[indiceArch].bloques_ind[bloqInd]->bloquesDatos[bloqDatos]->bytes + offset, '@', cantidadPadding);
				cantidadPadding = cantidadPadding - BLOCK_SIZE;
				bloqDatos++;
				offset = 0;

				int indBloque = ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER;//el indice en el bitmap arranca despues de eso
				int valorBit = bitarray_test_bit(bitmap, indBloque);

				if(tablaNodos[indiceArch].bloques_ind[bloqInd]->bloquesDatos[bloqDatos] == NULL)//si el sgte bloque es null
				{
					while(valorBit != 0 && indBloque < (ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER + cantBloqueDatos) )
					{
						indBloque++;
						valorBit = bitarray_test_bit(bitmap, indBloque);
					}
					bitarray_set_bit(bitmap, indBloque);

					tablaNodos[indiceArch].bloques_ind[0]->bloquesDatos[bloqDatos] = (GBlock*)tablaNodos + indBloque - ESTRUCTURAS_ADMIN;// + bloqDatos;

					loggearInfo("Puntero a bloque de datos seteado");
					loggearInfo("Bit seteado");
				}

			}
			bloqInd++;
		}
	}

	char* aux = malloc(50);
	sprintf(aux, "%d bytes de padding agregados", cantidadPadding);
	loggearInfo(aux);
	free(aux);

	tablaNodos[indiceArch].fecha_modif = time(NULL);
	tablaNodos[indiceArch].file_size = tamanioNuevo;
}

void recortarArchivo(int indiceArch, int tamanioNuevo)
{
	int tamActual = tablaNodos[indiceArch].file_size;
	int bloqInd = 0;
	int bloqDatos = 0;
	int aRecortar = tamActual - tamanioNuevo;
	int offset = 0;
	int indiceBloque = 0;
	int cortado = 0;
	int totalCortado = 0;

	if(tamActual < BLOCK_SIZE)
	{
		memset(tablaNodos[indiceArch].bloques_ind[0]->bloquesDatos[0]->bytes + tamanioNuevo, 0, BLOCK_SIZE - tamanioNuevo);

		if(aRecortar == tamActual)
		{
			indiceBloque = tablaNodos[indiceArch].bloques_ind[0]->bloquesDatos[0] - (GBlock*)tablaNodos + ESTRUCTURAS_ADMIN;
			bitarray_clean_bit(bitmap, indiceBloque);
			loggearInfo("Bit limpiado");

			indiceBloque = tablaNodos[indiceArch].bloques_ind[0] - (IndBlock*)tablaNodos + ESTRUCTURAS_ADMIN;
			bitarray_clean_bit(bitmap, indiceBloque);
			loggearInfo("Bit limpiado");

			tablaNodos[indiceArch].bloques_ind[0]->bloquesDatos[0] = NULL;
			tablaNodos[indiceArch].bloques_ind[0] = NULL;
		}

	}

	else
	{
		if(tamActual > (BLOCK_SIZE-1) )
		{
			bloqDatos = tamActual / BLOCK_SIZE;
			if(bloqDatos >= BLOQUES_DATOS)
			{
				bloqInd = bloqDatos/BLOQUES_DATOS;
				bloqDatos = bloqDatos % BLOQUES_DATOS;
			}

			offset = tamActual % BLOCK_SIZE;
			if(offset == 0 && bloqDatos != 0)
				bloqDatos--;
		}

		while(aRecortar > 0 && bloqInd >= 0)
		{

			while(aRecortar > 0 && bloqDatos >= 0)
			{
				if(bloqDatos == 0)
					offset = BLOCK_SIZE - aRecortar;
				cortado = BLOCK_SIZE - offset;
				memset(tablaNodos[indiceArch].bloques_ind[bloqInd]->bloquesDatos[bloqDatos]->bytes + offset, 0, cortado);
				aRecortar = aRecortar - cortado;
				offset = 0;
				totalCortado = totalCortado + cortado;

				if(cortado == BLOCK_SIZE)
				{
					indiceBloque = tablaNodos[indiceArch].bloques_ind[bloqInd]->bloquesDatos[bloqDatos] - (GBlock*)tablaNodos + ESTRUCTURAS_ADMIN;
					bitarray_clean_bit(bitmap, indiceBloque);
					loggearInfo("Bit limpiado");

					tablaNodos[indiceArch].bloques_ind[bloqInd]->bloquesDatos[bloqDatos] = NULL;
				}
				bloqDatos--;
			}

			if( ( (tamActual - totalCortado) % BLOCK_SIZE ) == 0 )
			{
				indiceBloque = tablaNodos[indiceArch].bloques_ind[bloqInd] - (IndBlock*)tablaNodos + ESTRUCTURAS_ADMIN;
				bitarray_clean_bit(bitmap, indiceBloque);
				loggearInfo("Bit limpiado");

				tablaNodos[indiceArch].bloques_ind[bloqInd] = NULL;
			}
			bloqInd--;
		}
	}

	if(aRecortar == tablaNodos[indiceArch].file_size)
		tablaNodos[indiceArch].bloques_ind[0] = NULL;

	char* aux = malloc(50);
	sprintf(aux, "%d bytes recortados", tamActual - tamanioNuevo);
	loggearInfo(aux);
	free(aux);

	tablaNodos[indiceArch].fecha_modif = time(NULL);
	tablaNodos[indiceArch].file_size = tamanioNuevo;

}

void escribir(void* bufferWrite)//(int indArchivo, char* contenido, size_t tamanio, off_t offset)
{
	char* nombre = malloc(MAX_FILENAME_LENGTH);
	int tamNom;
	size_t tamanio;

	off_t offset;


	memcpy(&tamNom, bufferWrite, sizeof(int) );
	memcpy(nombre, bufferWrite + sizeof(int), tamNom);
	memcpy(&tamanio, bufferWrite + sizeof(int) + tamNom, sizeof(size_t) );

	char* contenido = malloc(tamanio);
	memcpy(contenido, bufferWrite + sizeof(int) + tamNom + sizeof(size_t), tamanio);
	memcpy(&offset, bufferWrite + sizeof(int) + tamNom + sizeof(size_t) + tamanio, sizeof(off_t) );


	int indArchivo = indiceObjeto(nombre);


	int cantidadEscrita = 0;
	int bloqInd = 0;
	int bloqDatos = 0;
	int cantAEscribir;

	if(offset == 0)
		offset = tablaNodos[indArchivo].file_size;

	//sem_wait(&mutWrite);

	if(tablaNodos[indArchivo].file_size == 0)
	{
		//bloque de punteros indirectos


		int indBloque = ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER;//el indice en el bitmap arranca despues de eso
		int valorBit = bitarray_test_bit(bitmap, indBloque);
		while(valorBit != 0 && indBloque < (ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER + cantBloqueDatos) )
		{
			indBloque++;
			valorBit = bitarray_test_bit(bitmap, indBloque);
		}
		bitarray_set_bit(bitmap, indBloque);
		valorBit = bitarray_test_bit(bitmap, indBloque);

		tablaNodos[indArchivo].bloques_ind[0] =  (IndBlock*)tablaNodos + indBloque - ESTRUCTURAS_ADMIN;

		loggearInfo("Puntero a bloque indirecto seteado");
		loggearInfo("Bit seteado");

		//bloque de datos
		while(valorBit != 0 && indBloque < (ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER + cantBloqueDatos) )
		{
			indBloque++;
			valorBit = bitarray_test_bit(bitmap, indBloque);
		}
		bitarray_set_bit(bitmap, indBloque);

		tablaNodos[indArchivo].bloques_ind[0]->bloquesDatos[0] = (GBlock*)tablaNodos + indBloque - ESTRUCTURAS_ADMIN;

		loggearInfo("Puntero a bloque de datos seteado");
		loggearInfo("Bit seteado");

		memset(tablaNodos[indArchivo].bloques_ind[0]->bloquesDatos[0]->bytes, 0, BLOCK_SIZE);

		tablaNodos[indArchivo].bloques_ind[1] = NULL;

		int i = 1;
		while(i < BLOQUES_DATOS)
		{
			tablaNodos[indArchivo].bloques_ind[0]->bloquesDatos[i] = NULL;
			i++;
		}

	}

	if(tamanio + offset < BLOCK_SIZE)
	{
		memcpy(tablaNodos[indArchivo].bloques_ind[0]->bloquesDatos[0]->bytes + offset, contenido, tamanio);
		cantidadEscrita = tamanio;
	}
	else
	{

		if(offset > (BLOCK_SIZE-1) )
		{
			bloqDatos = offset / BLOCK_SIZE;
			if(bloqDatos >= BLOQUES_DATOS)
			{
				bloqInd = bloqDatos/BLOQUES_DATOS;
				bloqDatos = bloqDatos % BLOQUES_DATOS;
			}

			offset = offset % BLOCK_SIZE;
		}

		cantAEscribir = BLOCK_SIZE - offset;


		/////

		if(tamanio < BLOCK_SIZE)
			cantAEscribir = tamanio;

		while(cantidadEscrita < tamanio && bloqInd < BLOQUES_INDIRECTOS)
		{

			if(tablaNodos[indArchivo].bloques_ind[bloqInd] == NULL)
			{
				int indBloque = ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER;
				int valorBit = bitarray_test_bit(bitmap, indBloque);
				while(valorBit != 0 && indBloque < (ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER + cantBloqueDatos) )
				{
					indBloque++;
					valorBit = bitarray_test_bit(bitmap, indBloque);
				}
				bitarray_set_bit(bitmap, indBloque);
				valorBit = bitarray_test_bit(bitmap, indBloque);

				tablaNodos[indArchivo].bloques_ind[bloqInd] = (IndBlock*)tablaNodos + indBloque - ESTRUCTURAS_ADMIN;
				int i = 0;
				while(i < BLOQUES_DATOS)
				{
					tablaNodos[indArchivo].bloques_ind[bloqInd]->bloquesDatos[i] = NULL;
					i++;
				}

				loggearInfo("Puntero a bloque indirecto seteado");
				loggearInfo("Bit seteado");

				if(bloqInd < (BLOQUES_INDIRECTOS - 1) )
					tablaNodos[indArchivo].bloques_ind[bloqInd + 1] = NULL;
			}

			while( cantidadEscrita < tamanio && bloqDatos < BLOQUES_DATOS)
			{
				if(tablaNodos[indArchivo].bloques_ind[bloqInd]->bloquesDatos[bloqDatos] == NULL || tablaNodos[indArchivo].bloques_ind[bloqInd]->bloquesDatos[bloqDatos]->bytes == NULL)
				{
					int indBloque = ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER;
					int valorBit = bitarray_test_bit(bitmap, indBloque);
					while(valorBit != 0 && indBloque < (ESTRUCTURAS_ADMIN + MAX_FILE_NUMBER + cantBloqueDatos) )
					{
						indBloque++;
						valorBit = bitarray_test_bit(bitmap, indBloque);
					}
					bitarray_set_bit(bitmap, indBloque);

					tablaNodos[indArchivo].bloques_ind[bloqInd]->bloquesDatos[bloqDatos] = (GBlock*)tablaNodos + indBloque - ESTRUCTURAS_ADMIN;

					loggearInfo("Puntero a bloque de datos seteado");
					loggearInfo("Bit seteado");

					memset(tablaNodos[indArchivo].bloques_ind[bloqInd]->bloquesDatos[bloqDatos]->bytes, 0, BLOCK_SIZE);

				}
				memcpy(tablaNodos[indArchivo].bloques_ind[bloqInd]->bloquesDatos[bloqDatos]->bytes + offset, contenido+cantidadEscrita, cantAEscribir);

				cantidadEscrita = cantidadEscrita + cantAEscribir;
				cantAEscribir = BLOCK_SIZE;
				if( (tamanio-cantidadEscrita) < BLOCK_SIZE)
					cantAEscribir = tamanio-cantidadEscrita;
				bloqDatos++;
				offset = 0;

			}
			bloqInd++;
			bloqDatos = 0;
		}
	}

	char* aux = malloc(50);
	sprintf(aux, "%d bytes escritos", cantidadEscrita);
	loggearInfo(aux);
	free(aux);

	tablaNodos[indArchivo].fecha_modif = time(NULL);
	tablaNodos[indArchivo].file_size = tablaNodos[indArchivo].file_size + cantidadEscrita;//tamanio;

	msync(disco - 1 - BITMAP_SIZE_IN_BLOCKS, tamDisco, MS_SYNC);

	free(contenido);
	free(nombre);
	free(bufferWrite);

	//sem_post(&mutWrite);

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

	char* nombreViejo = malloc(MAX_FILENAME_LENGTH);
	char* padreViejo = malloc(MAX_FILENAME_LENGTH);
	char* nombreNuevo = malloc(MAX_FILENAME_LENGTH);
	char* padreNuevo = malloc(MAX_FILENAME_LENGTH);

	padreEHijo(oldpath, nombreViejo, padreViejo);
	padreEHijo(newpath, nombreNuevo, padreNuevo);

	sem_wait(&mutRename);

	int indiceViejo = indiceObjeto(nombreViejo);

	if( esArchivo(newpath) == 1) //si el nuevo ya existe, se renombra el viejo y se borra el nuevo
	{

		eliminarObjeto(nombreNuevo);

		memset(tablaNodos[indiceViejo].nombre, 0, MAX_FILENAME_LENGTH);

		strcpy(tablaNodos[indiceViejo].nombre, nombreNuevo);

	}
	else//si el nuevo no existe simplemente se renombra el viejo
	{

		memset(tablaNodos[indiceViejo].nombre, 0, MAX_FILENAME_LENGTH);

		strcpy(tablaNodos[indiceViejo].nombre, nombreNuevo);

		if(padreNuevo[0] != '\0')
			tablaNodos[indiceViejo].padre = indiceObjeto(padreNuevo);
		else
			tablaNodos[indiceViejo].padre = 0;

	}

	msync(disco - 1 - BITMAP_SIZE_IN_BLOCKS, tamDisco, MS_SYNC);

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
int esArchivo(char* path)
{

	char* nombre = malloc(MAX_FILENAME_LENGTH);
	char* padre = malloc(MAX_FILENAME_LENGTH);
	padreEHijo(path, nombre, padre);

	int indicePadre = indiceObjeto(padre);
	int indiceArch = indiceObjeto(nombre);

	if(indiceArch>=0 && indiceArch<=MAX_FILE_NUMBER)
	{
		if (tablaNodos[indiceArch].estado == ARCHIVO && tablaNodos[indiceArch].padre == indicePadre)
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

void eliminarObjeto(char* nombre)
{
	int indObjeto = indiceObjeto(nombre);
	int tam = tablaNodos[indObjeto].file_size;

	int indiceBloque = 0;

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
					indiceBloque = tablaNodos[indObjeto].bloques_ind[cont]->bloquesDatos[contDatos] - (GBlock*)tablaNodos + ESTRUCTURAS_ADMIN;//como explicar esta asquerosidad
					bitarray_clean_bit(bitmap, indiceBloque);
					loggearInfo("Bit limpiado");

					tablaNodos[indObjeto].bloques_ind[cont]->bloquesDatos[contDatos] = NULL;
				}
				contDatos++;
			}

			indiceBloque = tablaNodos[indObjeto].bloques_ind[cont] - (IndBlock*)tablaNodos + ESTRUCTURAS_ADMIN;
			bitarray_clean_bit(bitmap, indiceBloque);
			loggearInfo("Bit limpiado");

			tablaNodos[indObjeto].bloques_ind[cont] = NULL;
			cont++;
		}

		memset(tablaNodos[indObjeto].bloques_ind, 0 , BLOQUES_INDIRECTOS);
	}

	tablaNodos[indObjeto].estado = 0;
	memset(tablaNodos[indObjeto].nombre, 0, MAX_FILENAME_LENGTH);
	tablaNodos[indObjeto].file_size = 0;
	tablaNodos[indObjeto].padre = 0;
	bitarray_clean_bit(bitmap, ESTRUCTURAS_ADMIN + indObjeto);

	char* aux = malloc(50);
	sprintf(aux, "%d bytes eliminados", tam);
	loggearInfo(aux);
	free(aux);

	loggearInfo("Elemento eliminado");
	loggearInfo("Bit limpiado");

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

	if(estado == DIRECTORIO)
		loggearInfo("Directorio creado");
	else
		loggearInfo("Archivo creado");

	loggearInfo("Bit seteado");

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

	msync(disco - 1 - BITMAP_SIZE_IN_BLOCKS, tamDisco, MS_SYNC);

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
			int valorBit =bitarray_test_bit(bitmap, indActual + ESTRUCTURAS_ADMIN);
			if( valorBit != 0)
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

			char* nombre = malloc(200);
			recibirString(socketRespuesta, &nombre);//recibe nombre

			if(existeObjeto(nombre) == 1)//si existe envia su estado y ultima modificacion
			{

				int indice = indiceObjeto(nombre);
				uint8_t estado = tablaNodos[indice].estado;
				time_t ultimaMod = tablaNodos[indice].fecha_modif;
				uint32_t tamanio;

				if(estado == ARCHIVO)
					tamanio = tablaNodos[indice].file_size;
				else
					tamanio = 0;

				void* buffer = malloc( sizeof(int) + sizeof(time_t) + sizeof(uint32_t) );
				int tamBuffer = sizeof(time_t) + sizeof(uint32_t);

				enviarInt(socketRespuesta, estado);

				memcpy(buffer, &tamBuffer, sizeof(int) );
				memcpy(buffer + sizeof(int), &ultimaMod, sizeof(time_t) );
				memcpy(buffer + sizeof(int) + sizeof(time_t), &tamanio, sizeof(uint32_t));
				enviar(socketRespuesta, buffer, sizeof(int) + tamBuffer);

				free(buffer);

			}
			else
			{
				enviarInt(socketRespuesta, BORRADO);
			}

			free(nombre);

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


			indArch = indiceObjeto(nombreRead);

			enviarInt(socketRespuesta, indArch);

			if (indArch == -1)// si no existe
					break;

			int bloqInd = 0;
			int bloqDatos = 0;

			int cantALeer;
			int cantidadLeida = 0;

			if(tablaNodos[indArch].file_size == 0)
			{
				int tamAEnviar = 0;

				char* cont = strdup("");
				enviarInt(socketRespuesta, tamAEnviar);
				enviar(socketRespuesta, cont, tamAEnviar);

				free(nombreRead);
				free(buffer);

				break;
			}


			int suma = size + offset;

			if(suma > tablaNodos[indArch].file_size)
				size = tablaNodos[indArch].file_size - offset;

			//if(offset > tablaNodos[indArch].file_size)
				//cantALeer = 0;

			char* contAEnviar = malloc(size);

			if(size + offset < BLOCK_SIZE)
			{
				memcpy(contAEnviar, tablaNodos[indArch].bloques_ind[0]->bloquesDatos[0]->bytes + offset, size);
				cantidadLeida = size;
			}
			else
			{

				if(offset > (BLOCK_SIZE-1) )
				{
					bloqDatos = offset / BLOCK_SIZE;
					if(bloqDatos >= BLOQUES_DATOS)
					{
						bloqInd = bloqDatos/BLOQUES_DATOS;
						bloqDatos = bloqDatos % BLOQUES_DATOS;
					}

					offset = offset % BLOCK_SIZE;

				}

				cantALeer = BLOCK_SIZE - offset;


				while(cantidadLeida < size && bloqInd < BLOQUES_INDIRECTOS)
				{

					while( cantidadLeida < size && bloqDatos < BLOQUES_DATOS)
					{
						if(suma > tablaNodos[indArch].file_size && (tablaNodos[indArch].file_size - offset - BLOCK_SIZE) < 0)
						{
							if( (suma - tablaNodos[indArch].file_size) < BLOCK_SIZE)
								cantALeer = BLOCK_SIZE - (suma - tablaNodos[indArch].file_size);
							else
								cantALeer = BLOCK_SIZE - ( (suma - tablaNodos[indArch].file_size) % BLOCK_SIZE );
						}

						memcpy(contAEnviar + cantidadLeida, tablaNodos[indArch].bloques_ind[bloqInd]->bloquesDatos[bloqDatos]->bytes + offset, cantALeer);
						cantidadLeida = cantidadLeida + cantALeer;
						cantALeer = BLOCK_SIZE;

						if( (size-cantidadLeida) < BLOCK_SIZE)
							cantALeer = size-cantidadLeida;

						bloqDatos++;
						offset = 0;

					}
					bloqInd++;
					bloqDatos = 0;
				}
			}

			int tamAEnviar = cantidadLeida;

			enviarInt(socketRespuesta, tamAEnviar);
			enviar(socketRespuesta, contAEnviar, tamAEnviar);

			free(nombreRead);
			free(buffer);
			//free(contAEnviar);

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
			recibir(socketRespuesta, &tamARecibir, sizeof(int) );

			void* bufferWrite = malloc(tamARecibir);
			recibir(socketRespuesta, bufferWrite, tamARecibir);

		/*	char* nombre = malloc(MAX_FILENAME_LENGTH);
			int tamNom;
			size_t tamContenido;

			off_t offset;


			memcpy(&tamNom, bufferWrite, sizeof(int) );
			memcpy(nombre, bufferWrite + sizeof(int), tamNom);
			memcpy(&tamContenido, bufferWrite + sizeof(int) + tamNom, sizeof(size_t) );

			char* contenido = malloc(tamContenido);
			memcpy(contenido, bufferWrite + sizeof(int) + tamNom + sizeof(size_t), tamContenido);
			memcpy(&offset, bufferWrite + sizeof(int) + tamNom + sizeof(size_t) + tamContenido, sizeof(off_t) );


			indArch = indiceObjeto(nombre);*/
		//	if (indArch == -1)//ver
		//			break;

			int hiloWrite = 0;
			//escribir(indArch, contenido, tamContenido, offset);
			if( ( hiloWrite = makeDetachableThread(escribir, bufferWrite) ) == 0)
				loggearError("Error al crear un nuevo hilo");

		//	free(contenido);
		//	free(nombre);
			//free(bufferWrite);

			break;

		}

		case RENAME:
		{

			int tamARecibir;
			recibir(socketRespuesta,&tamARecibir, sizeof(int) );

			void* pathsRecibidos = malloc(tamARecibir);
			recibir(socketRespuesta, pathsRecibidos, tamARecibir);

			int tamOld;
			int tamNew;

			memcpy(&tamOld, pathsRecibidos, sizeof(int));
			char* oldpath = malloc(tamOld);

			memcpy(oldpath, pathsRecibidos + sizeof(int), tamOld);

			memcpy(&tamNew,pathsRecibidos + sizeof(int) + tamOld, sizeof(int) );
			char* newpath = malloc(tamNew);

			memcpy(newpath, pathsRecibidos + sizeof(int) + tamOld + sizeof(int), tamNew);


			//ERRORES
/*
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
*/
			//FUNCIONALIDAD

			if(strcmp(oldpath,newpath) == 0)//si son iguales no se hace nada y se devuelve 0
			{
				enviarInt(socketRespuesta, 0);
				break;
			}

			renombrar(oldpath, newpath);

			enviarInt(socketRespuesta, 0);

			char* nombreViejo = nombreObjeto(oldpath);

			if(esDirectorio(nombreViejo) == 1)
				loggearInfo("Directorio renombrado");
			else
				loggearInfo("Archivo renombrado");

			free(oldpath);
			free(newpath);
			free(pathsRecibidos);

			break;
		}
		case UNLINK:
		{
			int error;
			char* pathUnlink = malloc(200);
			char* nombre = malloc(MAX_FILENAME_LENGTH);
			char* padre = malloc(MAX_FILENAME_LENGTH);


			recibirString(socketRespuesta, &pathUnlink);//recibe el path

			padreEHijo(pathUnlink, nombre, padre);

			if(esDirectorio(nombre) == 1)//si es directorio tira error
			{
				error = EISDIR;
				enviarInt(socketRespuesta, error);
				break;
			}

			/*if(esArchivo(pathUnlink) == 0)//si no es archivo tira error
			{
				error = ENOENT;
				enviarInt(socketRespuesta, error);
				break;
			}*/

			eliminarObjeto(nombre);


			enviarInt(socketRespuesta, 0);//avisa que esta todo piola
			free(pathUnlink);

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
		case TRUNCATE:
		{
			char* pathTruncate = malloc(200);
			off_t tamanioNuevo;
			int tamanioPath;
			int tamRecibir;

			recibir(socketRespuesta,&tamRecibir, sizeof(int) );

			void* bufferWrite = malloc(tamRecibir);
			recibir(socketRespuesta, bufferWrite, tamRecibir);

			memcpy(&tamanioPath, bufferWrite, sizeof(int));
			memcpy(pathTruncate, bufferWrite + sizeof(int), tamanioPath);
			memcpy(&tamanioNuevo,bufferWrite + sizeof(int) + tamanioPath, sizeof(off_t) );

			char* nombre = nombreObjeto(pathTruncate);
			int indiceArch = indiceObjeto(nombre);

			if(tamanioNuevo == tablaNodos[indiceArch].file_size)
			{
				//hacer algo
			}


			if(tamanioNuevo < tablaNodos[indiceArch].file_size)
			{
				recortarArchivo(indiceArch, tamanioNuevo);
			}
			else
				rellenarArchivo(indiceArch, tamanioNuevo);

			free(pathTruncate);

			int resultado = 0;

			enviarInt(socketRespuesta, resultado);

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

	void* punteroBitmap = (char*) disco + BLOCK_SIZE;//(void*) disco + 1;
	bitmap = bitarray_create_with_mode(punteroBitmap, tamBitmap, MSB_FIRST);//(char*) disco + BLOCK_SIZE


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




