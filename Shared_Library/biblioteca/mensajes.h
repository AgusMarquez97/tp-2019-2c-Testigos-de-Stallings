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



// -----------------


//Enviar y recibir datos primitivos:

void enviarInt(int socketReceptor, int32_t entero);
void enviarChar(int socketReceptor, char caracter);
void enviarString(int socketReceptor, char* cadena);
void enviarUint(int socketReceptor, uint32_t entero);
void enviarVoid(int socketReceptor, void * buffer_origen, int32_t cantidadBytes);

int recibirInt(int socketEmisor, int32_t* entero);
int recibirChar(int socketEmisor, char* caracter);
int recibirString(int socketEmisor, char** cadena);
int recibirUint(int socketEmisor, uint32_t* entero);
int recibirVoid(int socketEmisor, void ** buffer);













#endif /* MENSAJES_H_ */
