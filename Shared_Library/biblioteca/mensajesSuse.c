/*
 * mensajesSuse.c
 *
 *  Created on: Oct 6, 2019
 *      Author: agus
 */


#include "mensajesSuse.h"


void enviarCreate(int socketReceptor, int32_t proceso, int32_t tid) {
	enviarOperacionSuse(socketReceptor, proceso, CREATE, tid, 0,NULL);
}

void enviarJoin(int socketReceptor, int32_t proceso, int32_t tid) {
	enviarOperacionSuse(socketReceptor, proceso, JOIN, tid, 0,NULL);
}

void enviarNext(int socketReceptor, int32_t proceso) {
	enviarOperacionSuse(socketReceptor, proceso, NEXT, -1, 0,NULL);
}

void enviarCloseSuse(int socketReceptor, int32_t proceso, int32_t tid) {
	enviarOperacionSuse(socketReceptor, proceso, CLOSE_SUSE, tid,0,NULL);
}

void enviarWait (int socketReceptor, int32_t proceso, int32_t tid, char * semId){
	enviarOperacionSuse(socketReceptor, proceso, WAIT, tid,0,semId);
}

void enviarSignal (int socketReceptor, int32_t proceso, int32_t tid, char * semId){
	enviarOperacionSuse(socketReceptor, proceso, SIGNAL, tid,0,semId);
}


void enviarOperacionSuse(int socket, int32_t proceso, int32_t operacion, int32_t tid,
		int32_t rafaga, char* semId) {

	int32_t desplazamiento = 0;
	int32_t tamanioBuffer = sizeof(int32_t) * 2;

	if(tid >= 0) //porque los hilos tienen id postivo
		tamanioBuffer += sizeof(int32_t);
	if(rafaga != 0)
		tamanioBuffer += sizeof(int32_t);
	if(semId != NULL)
		tamanioBuffer += strlen(semId)*sizeof(semId); //si le doy solo strlen(semId) no alcanza, pero tampoco le puedo dar algo como +1 xq queda feo


	void* buffer = malloc(tamanioBuffer);

	serializarInt(buffer, proceso , &desplazamiento);
	serializarInt(buffer, operacion, &desplazamiento);

	switch(operacion) {
		case CREATE:
			serializarInt(buffer, tid, &desplazamiento);
			break;
		case NEXT:
			//
			break;
		case JOIN:
			serializarInt(buffer, tid, &desplazamiento);
			break;
		case CLOSE_SUSE:
			serializarInt(buffer, tid, &desplazamiento);
			break;
		case WAIT:
			serializarInt(buffer, tid, &desplazamiento);
			serializarVoid(buffer,semId,strlen(semId)+1,&desplazamiento); //aca si el tamanio es strlen
			break;
		case SIGNAL:
			serializarInt(buffer, tid, &desplazamiento);
			serializarVoid(buffer,semId,strlen(semId)+1,&desplazamiento); //aca si el tamanio es strlen
			break;
		default: // la funcion no es void???
			; //return NULL;  //Comento esto para que compile
	}

	enviar(socket, buffer, tamanioBuffer);
	free(buffer);

}

t_mensajeSuse* recibirOperacionSuse(int socketEmisor) {

	t_mensajeSuse* mensajeRecibido;
	int cantidadRecibida = 0;
	int32_t  proceso, operacion;
	int tam=0;
	int desplazamiento=0;
	void* buffer;
	cantidadRecibida = recibirInt(socketEmisor, &proceso);
	cantidadRecibida += recibirInt(socketEmisor, &operacion);

	if(cantidadRecibida != (sizeof(int32_t)*2) )
		return NULL;

	mensajeRecibido = malloc(sizeof(mensajeRecibido));

	mensajeRecibido->idProceso = proceso;
	mensajeRecibido->tipoOperacion = operacion;

	switch(operacion) {
		case CREATE:
			recibirInt(socketEmisor, &mensajeRecibido->idHilo);
			break;
		case NEXT:
			//
			break;
		case JOIN:
			recibirInt(socketEmisor, &mensajeRecibido->idHilo);
			break;
		case CLOSE_SUSE:
			recibirInt(socketEmisor, &mensajeRecibido->idHilo);
			break;
		case WAIT:
			recibirInt(socketEmisor, &mensajeRecibido->idHilo);
			recibirInt(socketEmisor, &tam);
			buffer = malloc(tam);
			recibir(socketEmisor, buffer, tam);
			deserializarVoid((char*)buffer, &mensajeRecibido->semId, tam, &desplazamiento);

			break;
		case SIGNAL:
			recibirInt(socketEmisor, &mensajeRecibido->idHilo);
			recibirInt(socketEmisor, &tam);
			buffer = malloc(tam);
			recibir(socketEmisor, buffer, tam);
			deserializarVoid((char*)buffer, &mensajeRecibido->semId, tam, &desplazamiento);
			break;
		default:
			return NULL;
	}

	return mensajeRecibido;

}

