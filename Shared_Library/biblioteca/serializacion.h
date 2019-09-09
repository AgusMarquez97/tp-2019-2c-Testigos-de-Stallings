#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/collections/list.h>

#include "logs.h"

//Serializaciones primitivas

void serializarInt(void* buffer, int32_t entero, int* desplazamiento);
void serializarDouble(void* buffer, int64_t numero, int* desplazamiento);
void serializarChar(void* buffer, char caracter, int* desplazamiento);
void serializarString(void* buffer, char* cadena, int* desplazamiento);

void deserializarInt(void* buffer,int32_t* entero,int* desplazamiento);
void deserializarDouble(void* buffer,int64_t* entero,int* desplazamiento);
void deserializarChar(void* buffer,char* caracter,int* desplazamiento);
void deserializarString(void* buffer,char** cadena,int* desplazamiento);

/*
int32_t requestType;
char* tabla;
int32_t key;
char* value;
int64_t timestamp;
*/
#endif /* SERIALIZACION_H_ */
