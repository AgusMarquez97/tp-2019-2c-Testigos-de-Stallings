/*
 * mensajesSuse.c
 *
 *  Created on: Oct 6, 2019
 *      Author: agus
 */


#include "mensajesSuse.h"


void enviarHandshake(int socketReceptor, int32_t proceso) {
	enviarOperacion(socketReceptor, proceso, HANDSHAKE, -1,0);
}

void enviarCreate(int socketReceptor, int32_t proceso, int32_t tid) {
	enviarOperacion(socketReceptor, proceso, CREATE, tid, 0);
}

void enviarJoin(int socketReceptor, int32_t proceso, int32_t tid) {
	enviarOperacion(socketReceptor, proceso, JOIN, tid, 0);
}

void enviarNext(int socketReceptor, int32_t proceso) {
	enviarOperacion(socketReceptor, proceso, NEXT, -1, 0);
}

void enviarReturn(int socketReceptor, int32_t proceso) {
	enviarOperacion(socketReceptor, proceso, RETURN, -1,0);
}

void enviarOperacion(int socket, int32_t proceso, int32_t operacion, int32_t tid,
		int32_t rafaga) {

	int32_t desplazamiento = 0;
	int32_t tamanioBuffer = sizeof(int32_t) * 2;

	if(tid >= 0) //porque los hilos tienen id postivo
		tamanioBuffer += sizeof(int32_t);
	if(rafaga != 0)
		tamanioBuffer += sizeof(int32_t);


	void* buffer = malloc(tamanioBuffer);

	serializarInt(buffer, proceso , &desplazamiento);
	serializarInt(buffer, operacion, &desplazamiento);

	switch(operacion) {
		case HANDSHAKE_SUSE:
			//serializarInt(buffer, flag, &desplazamiento);
			break;
		case CREATE:
			serializarInt(buffer, tid, &desplazamiento);
			serializarInt(buffer, rafaga, &desplazamiento);
			break;
		case NEXT:
			//
			break;
		case JOIN:
			serializarInt(buffer, tid, &desplazamiento);
			break;
		case RETURN:
			//
			break;

		default: // la funcion no es void???
			return NULL;
	}

	enviar(socket, buffer, tamanioBuffer);
	free(buffer);

}

t_mensajeSuse* recibirOperacion(int socketEmisor) {

	t_mensajeSuse* mensajeRecibido;
	int cantidadRecibida = 0;
	int desplazamiento = 0;
	int32_t proceso, operacion;
	void* buffer = NULL;

	cantidadRecibida = recibirInt(socketEmisor, proceso);
	cantidadRecibida += recibirInt(socketEmisor, operacion);

	if(cantidadRecibida =! sizeof(int32_t)*2)
		return NULL;

	mensajeRecibido = malloc(sizeof(mensajeRecibido));

	mensajeRecibido->idProceso = proceso;
	mensajeRecibido->tipoOperacion = operacion;

	switch(operacion) {
		case HANDSHAKE_SUSE:
			//recibirInt(socketEmisor, &mensajeRecibido->flag);
			break;
		case CREATE:
			recibirInt(socketEmisor, &mensajeRecibido->idHilo);
			recibirInt(socketEmisor, &mensajeRecibido->rafaga);
			break;
		case NEXT:
			//
			break;
		case JOIN:
			recibirInt(socketEmisor, &mensajeRecibido->idHilo);
			break;
		case RETURN:
			//
			break;
		default:
			return NULL;
	}

	return mensajeRecibido;

}
