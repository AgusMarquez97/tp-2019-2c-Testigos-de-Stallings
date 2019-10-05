#ifndef MENSAJES_H_
#define MENSAJES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "utils.h"
#include "enumsAndStructs.h"
#include "serializacion.h"
#include "sockets.h"
#include "logs.h"

/*
 * MENSAJES MUSE
 */


void enviarMalloc(int socketReceptor,int32_t id_proceso, int32_t tam);
void enviarFree(int socketReceptor,int32_t id_proceso, uint32_t posicion);
void enviarUnmap(int socketReceptor,int32_t id_proceso, uint32_t posicion);
void enviarGet(int socketReceptor,int32_t id_proceso, uint32_t posicionMuse, int32_t cantidadBytes);
void enviarSync(int socketReceptor,int32_t id_proceso, uint32_t posicionMuse, int32_t cantidadBytes);
void enviarCpy(int socketReceptor,int32_t id_proceso, void * origen, int32_t cantidadBytes);
void enviarMap(int socketReceptor,int32_t id_proceso, char * contenidoArchivo, int32_t flag);

mensajeMuse * recibirRequestMuse(int socketEmisor);

// -----------------


//Enviar y recibir datos primitivos:

void enviarInt(int socketReceptor, int32_t entero);
void enviarChar(int socketReceptor, char caracter);
void enviarString(int socketReceptor, char* cadena);
void enviarUint(int socketReceptor, uint32_t entero);

int recibirInt(int socketEmisor, int32_t* entero);
int recibirChar(int socketEmisor, char* caracter);
int recibirString(int socketEmisor, char** cadena);
int recibirUint(int socketEmisor, uint32_t* entero);













#endif /* MENSAJES_H_ */
