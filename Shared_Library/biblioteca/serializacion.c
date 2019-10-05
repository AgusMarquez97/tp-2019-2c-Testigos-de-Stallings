#include "serializacion.h"


//SERIALIZAR Y DESERIALIZAR DATOS PRIMITIVOS


void serializarUint(void* buffer, uint32_t entero, int* desplazamiento) {

	memcpy(buffer + *desplazamiento, &entero, sizeof(uint32_t));
	*desplazamiento += sizeof(uint32_t);
}

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

void serializarVoid(void* buffer, void * origen,int32_t cantidadBytes, int* desplazamiento)
{
	serializarInt(buffer,cantidadBytes, desplazamiento); // tamanio del buffer

	memcpy(buffer + *desplazamiento, origen, cantidadBytes);
	*desplazamiento += cantidadBytes;
}




void deserializarInt(void* buffer,int32_t* entero,int* desplazamiento) {

	memcpy(entero, buffer + *desplazamiento, sizeof(int32_t));
	*desplazamiento += sizeof(int32_t);
}

void deserializarUint(void* buffer,uint32_t* entero,int* desplazamiento) {

	memcpy(entero, buffer + *desplazamiento, sizeof(uint32_t));
	*desplazamiento += sizeof(uint32_t);
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

void deserializarVoid(void* buffer, void ** dst, int cantidadBytes, int* desplazamiento)
{

	*dst = malloc(cantidadBytes);

	memcpy(*dst,buffer + *desplazamiento,cantidadBytes);

	*desplazamiento += cantidadBytes;
}



//SERIALIZAR Y DESERIALIZAR DATOS PRIMITIVOS






