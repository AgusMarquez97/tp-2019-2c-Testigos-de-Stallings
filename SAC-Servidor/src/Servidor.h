#ifndef SERVIDOR_H_
#define SERVIDOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <biblioteca/sockets.h>
#include <biblioteca/serializacion.h>
#include <biblioteca/mensajes.h>
#include <biblioteca/mensajesFuse.h>
#include <biblioteca/enumsAndStructs.h>
#include <biblioteca/logs.h>
#include <biblioteca/levantarConfig.h>
#include <biblioteca/utils.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/node.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>
#include <commons/bitarray.h>

#define pathConfig "/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/SAC-Servidor/config/configuracion.txt"

#define IDENTIFICADOR 3
#define BLOCK_SIZE 4096
#define MAX_FILE_NUMBER 10//1024
#define MAX_FILENAME_LENGTH 71
#define BITMAP_START_BLOCK 1
#define BITMAP_SIZE_IN_BLOCKS 1

typedef uint32_t ptrGBloque;


char ip[46];
char puerto[10];

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

/*
 * Levanta los datos de la estructura config y los guarda en las variables globales que corresponda
 */
void levantarConfig();


/*
 * Para el servidor:
 * 1) Se crea un socket servidor que siempre estara a la escucha de nuevas conexiones
 * 2) Se crea un socket respuesta donde se acepta una conexion
 * 3) Se crea un hilo que recibe el socket de la conexion por parametro -> esto es lo que provoca la concurrencia y la necesidad de sincronizar
 * 4) Se reciben los datos pertinentes y se responde como se crea necesario (siempre usando el socket rta para enviar/recibir)
 *
 */
void levantarServidorFUSE();
void rutinaServidor(int socketRespuesta);

/*
 * Libera variables globales y memoria alocada que normalmente no se libera. -> Ver de usar signal para atrapar una salida forzada
 */
void liberarVariablesGlobales();


#endif
