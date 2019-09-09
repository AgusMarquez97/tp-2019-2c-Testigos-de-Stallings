#include "serializacion.h"


//SERIALIZAR Y DESERIALIZAR DATOS PRIMITIVOS

void serializarInt(void* buffer, int32_t entero, int* desplazamiento) {

	memcpy(buffer + *desplazamiento, &entero, sizeof(int32_t));
	*desplazamiento += sizeof(int32_t);
}

void serializarDouble(void* buffer, int64_t numero, int* desplazamiento) {

	memcpy(buffer + *desplazamiento, &numero, sizeof(int64_t));
	*desplazamiento += sizeof(int64_t);

}

void serializarChar(void* buffer, char caracter, int* desplazamiento) {

	memcpy(buffer + *desplazamiento, &caracter, sizeof(char));
	*desplazamiento += sizeof(char);
}

void serializarString(void* buffer, char* cadena, int* desplazamiento) {


	serializarInt(buffer,strlen(cadena) + 1, desplazamiento);

	memcpy(buffer + *desplazamiento, cadena, strlen(cadena) + 1);
	*desplazamiento += strlen(cadena) + 1;

}

void deserializarInt(void* buffer,int32_t* entero,int* desplazamiento) {

	memcpy(entero, buffer + *desplazamiento, sizeof(int32_t));
	*desplazamiento += sizeof(int32_t);

}

void deserializarDouble(void* buffer,int64_t* entero,int* desplazamiento) {

	memcpy(entero, buffer + *desplazamiento, sizeof(int64_t));
	*desplazamiento += sizeof(int64_t);

}

void deserializarChar(void* buffer,char* caracter,int* desplazamiento){

	memcpy(caracter, buffer + *desplazamiento, sizeof(char));
	*desplazamiento += sizeof(char);

}

void deserializarString(void* buffer,char** cadena,int* desplazamiento){

	int32_t tamanioCadena = 0;

	deserializarInt(buffer,&tamanioCadena, desplazamiento);

	*cadena = malloc(tamanioCadena);

	memcpy(*cadena,buffer + *desplazamiento, tamanioCadena);

	*desplazamiento += strlen(*cadena) + 1;

}

//SERIALIZAR Y DESERIALIZAR DATOS PRIMITIVOS






