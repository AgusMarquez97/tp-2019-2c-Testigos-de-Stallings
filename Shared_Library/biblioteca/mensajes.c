#include "mensajes.h"

void enviarUint(int socketReceptor, uint32_t entero) {

	int desplazamiento = 0;
	void* buffer = malloc(sizeof(uint32_t));

	serializarUint(buffer, entero, &desplazamiento);
	enviar(socketReceptor, buffer, sizeof(uint32_t));

	free(buffer);

}

void enviarInt(int socketReceptor, int32_t entero) {

	int desplazamiento = 0;
	void* buffer = malloc(sizeof(int32_t));

	serializarInt(buffer, entero, &desplazamiento);
	enviar(socketReceptor, buffer, sizeof(int32_t));

	free(buffer);

}

void enviarChar(int socketReceptor, char caracter) {

	int desplazamiento = 0;
	void* buffer = malloc(sizeof(char));

	serializarChar(buffer, caracter, &desplazamiento);
	enviar(socketReceptor, buffer, sizeof(char));

	free(buffer);

}

void enviarString(int socketReceptor, char* cadena) {

	int desplazamiento = 0;

	if(cadena == NULL)
		cadena = strdup("VOID");

	int32_t tamanioCadena = sizeof(int32_t) + strlen(cadena) + 1;
	int32_t tamanioBuffer = sizeof(int32_t) + tamanioCadena;

	void* buffer = malloc(tamanioBuffer);

	serializarInt(buffer, tamanioCadena, &desplazamiento);
	serializarString(buffer, cadena, &desplazamiento);
	enviar(socketReceptor, buffer, tamanioBuffer);

	free(buffer);
	free(cadena);

}

void enviarVoid(int socketReceptor, void * bufferOrigen, int32_t cantidadBytes) {

	int desplazamiento = 0;
	int32_t tamanioBuffer = sizeof(int32_t) + cantidadBytes;

	void* bufferDestino = malloc(tamanioBuffer);

	serializarVoid(bufferDestino, bufferOrigen, cantidadBytes, &desplazamiento);
	enviar(socketReceptor, bufferDestino, tamanioBuffer);

	free(bufferDestino);

}

/*
 * 	RECIBIR DATOS PRIMITIVOS
 */

int recibirUint(int socketEmisor, uint32_t* entero) {

	int desplazamiento = 0;
	void* buffer = malloc(sizeof(uint32_t));

	int cantidadRecibida = recibir(socketEmisor, buffer, sizeof(uint32_t));
	deserializarUint(buffer, entero, &desplazamiento);

	free(buffer);
	return cantidadRecibida;

}

int recibirInt(int socketEmisor, int32_t* entero) {

	int desplazamiento = 0;
	void* buffer = malloc(sizeof(int32_t));

	int cantidadRecibida = recibir(socketEmisor, buffer, sizeof(int32_t));
	deserializarInt(buffer, entero, &desplazamiento);

	free(buffer);
	return cantidadRecibida;

}

int recibirChar(int socketEmisor, char* caracter) {

	int desplazamiento = 0;
	void* buffer = malloc(sizeof(char));

	int cantidadRecibida = recibir(socketEmisor, buffer, sizeof(char));
	deserializarChar(buffer, caracter, &desplazamiento);

	free(buffer);
	return cantidadRecibida;

}

int recibirString(int socketEmisor, char** cadena) {

	int desplazamiento = 0;
	int tamBuffer = 0;
	void * buffer = malloc(sizeof(int32_t));

	int cantidadRecibida = recibir(socketEmisor, buffer, sizeof(int32_t));
	deserializarInt(buffer, &tamBuffer, &desplazamiento);

	buffer = realloc(buffer, tamBuffer);
	cantidadRecibida += recibir(socketEmisor, buffer, tamBuffer);
	desplazamiento = 0;
	deserializarString(buffer, cadena, &desplazamiento);

	free(*cadena);
	free(buffer);
	return cantidadRecibida;

}


int recibirVoid(int socketEmisor, void** bufferDestino) {

	int desplazamiento = 0;
	int32_t tamanioBuffer;

	int cantidadRecibida = sizeof(int32_t);
	recibirInt(socketEmisor, &tamanioBuffer);
	void* bufferRecibido = malloc(tamanioBuffer);

	cantidadRecibida += recibir(socketEmisor, bufferRecibido, tamanioBuffer);

	if(cantidadRecibida == sizeof(int32_t)) {
		free(bufferRecibido);
		return -1;
	}

	deserializarVoid(bufferRecibido, bufferDestino, tamanioBuffer, &desplazamiento);

	free(bufferRecibido);
	return cantidadRecibida;

}
