/*
 * formatter.h
 *
 *  Created on: 26 oct. 2019
 *      Author: utnso
 */

#ifndef FORMATTER_H_
#define FORMATTER_H_

#define IDENTIFICADOR 3
#define MAGIC_NUMBER_NAME 3
#define BLOCK_SIZE 4096
#define MAX_FILE_NUMBER 10//1024
#define MAX_FILENAME_LENGTH 71
#define BITMAP_START_BLOCK 1
#define BITMAP_SIZE_IN_BLOCKS 1

typedef uint32_t ptrGBloque;

typedef struct bloque
{
	unsigned char bytes[BLOCK_SIZE];
}GBlock;

typedef struct header
{
	unsigned char sac[IDENTIFICADOR];
	uint32_t version;
	uint32_t bitmap_start;
	uint32_t bitmap_size;
	unsigned char padding[4081];
}GHeader;


typedef struct archivo
{
	uint8_t estado; //0:borrado, 1:archivo, 2:directorio
	char nombre[MAX_FILENAME_LENGTH];
	uint32_t file_size;
	char contenido[256];
	struct archivo* padre;
}GFile;


#endif /* FORMATTER_H_ */
