#include <stddef.h>
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <limits.h>

#include "formatter.h"

void escribirTablaNodos(GBlock* punteroDisco)
{
	struct archivo* tablaNodos = (struct archivo*) punteroDisco;

	for(int nArch = 0; nArch < MAX_FILE_NUMBER; nArch++)
		tablaNodos[nArch].estado = 0;
}

char* nombrePorCodigo(int codigo)
{
	if (codigo == 0)
		return "Libre";
	else if (codigo == 1)
		return "Archivo";
	else
		return "Directorio";
}

void dumpTablaNodos(GBlock *ptrDisco)
{
	struct archivo* tablaNodos = (struct archivo*) ptrDisco;

	char* nombreAux = malloc(MAX_FILENAME_LENGTH+1);

	for(int contador = 0; contador < MAX_FILE_NUMBER; contador++)
	{
		GFile nodo = tablaNodos[contador];
		memset(nombreAux, '\0', (MAX_FILENAME_LENGTH + 1) * sizeof(char) );
		memcpy(nombreAux, nodo.nombre, MAX_FILENAME_LENGTH);

		if(nodo.estado>0)
		{
			printf("Nodo %u - Estado: %s\tNombre: %s\tTamanio: %u\n", contador, nombrePorCodigo(nodo.estado), nodo.nombre , nodo.file_size);
		}
		else
		{
			printf("Nodo %u - Estado: %s\n", contador, nombrePorCodigo(nodo.estado));
		}
	}
	free(nombreAux);

}

void escribirBitmap(GBlock* ptrDisco)
{
	int* bytesBloque = (int*) ptrDisco;
	memset(bytesBloque, '\0', BLOCK_SIZE * BITMAP_SIZE_IN_BLOCKS);

	int bits = 1 + BITMAP_SIZE_IN_BLOCKS + MAX_FILE_NUMBER;

	for(int cont=0; cont< bits; cont++)
	{
		bytesBloque[cont/CHAR_BIT] |=1 << (cont% CHAR_BIT);
	}
}

void dumpBitmap(GBlock* ptrDisco)
{
	/*int* bytesBloque = (int*) ptrDisco;

	uint32_t usados = 0;
	uint32_t totalBloques = BLOCK_SIZE * BITMAP_SIZE_IN_BLOCKS;


	for(int bit = 0; bit < totalBloques; bit++)
	{
		int bitActual = ((bytesBloque[bit / CHAR_BIT] >> (bit % CHAR_BIT)) );//?
		printf("%u", bitActual);
		if(bitActual)
			usados++;

	}

	printf("\n\nTotal de bloques: %u\tUsados: %u\tLibres: %u\n", totalBloques, usados, totalBloques-usados);*/
	printf("Asumi que anda. Revisar con tiempo");

}

void escribirHeader(GBlock* ptrDisco){
	struct header* nuevoHeader = (struct header*) ptrDisco;

	memcpy(nuevoHeader->sac, "SAC", MAGIC_NUMBER_NAME);
	nuevoHeader->bitmap_size = BITMAP_SIZE_IN_BLOCKS;
	nuevoHeader->bitmap_start = BITMAP_START_BLOCK;
	nuevoHeader->version = 1;
}

void dumpHeader(GBlock* ptrDisco)
{
	struct header* nuevoHeader = (struct header*) ptrDisco;

	char* nombreAux = malloc(MAGIC_NUMBER_NAME +1);
	memset(nombreAux,0,MAGIC_NUMBER_NAME +1 * sizeof(char));
	memcpy(nombreAux, nuevoHeader->sac, MAGIC_NUMBER_NAME);

	printf("\tMagic Number: %s\n", nombreAux);
	printf("\tTamaño de Bitmap: %u\n", nuevoHeader->bitmap_size);
	printf("\tComienzo de Bitmap: %u\n", nuevoHeader->bitmap_start);
	printf("\tVersion: %u\n", nuevoHeader->version);

	free(nombreAux);
}

size_t tamArchivo(char* archivo)
{
	FILE* fd = fopen(archivo, "r");

	fseek(fd, 0L, SEEK_END);
	uint32_t tamanio = ftell(fd);

	fclose(fd);
	return tamanio;
}


int main(int argc, char** argv)
{
	char* nombreArchivo;
	bool debeFormatear;
	if(argc == 3 && (strcmp(argv[1], "--format") || (strcmp(argv[1],"-f"))))
	{
		debeFormatear = true;
		nombreArchivo = argv[2];
	} else
	{
		debeFormatear = false;
		nombreArchivo = argv[1];
	}

	size_t tamanioDisco = tamArchivo(nombreArchivo);
	int discoFD = open(nombreArchivo, O_RDWR, 0);
	GBlock* disco = mmap(NULL, tamanioDisco, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, discoFD, 0);

	if(debeFormatear)
	{
		printf("Formateando disco %s...\n", nombreArchivo);

		printf("\tEscribiendo header en posicion %p\n", disco);
		escribirHeader(disco);

		printf("\tEscribiendo bitmap en posicion %p\n", disco+1);
		escribirBitmap(disco+1);

		printf("\tEscribiendo tabla de nodos en posicion %p\n", disco + 1 + BITMAP_SIZE_IN_BLOCKS);
		escribirTablaNodos(disco + 1 + BITMAP_SIZE_IN_BLOCKS);

		printf("HECHO\n");
	}
	else
	{
		printf("Dumpeando disco  %s...\n\n", nombreArchivo);
		printf("Contenido del header:\n");
		dumpHeader(disco);

		printf("\n\nContenido del bitmap:\n");
		dumpBitmap(disco + 1);

		printf("\n\nContenido de la tabla de nodos:\n");
		dumpTablaNodos(disco +1 + BITMAP_SIZE_IN_BLOCKS);

	};

	munmap(disco, tamanioDisco);
	close(discoFD);
	return 0;

}
