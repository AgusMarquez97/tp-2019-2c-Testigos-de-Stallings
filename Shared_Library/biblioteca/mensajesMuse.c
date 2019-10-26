#include "mensajesMuse.h"

void enviarHandshake(int socketReceptor, int32_t proceso) {
	enviarOperacion(socketReceptor, proceso, HANDSHAKE, 0, 0, NULL, NULL, 1);
}

void enviarMalloc(int socketReceptor, int32_t proceso, int32_t tamanio) {
	enviarOperacion(socketReceptor, proceso, MALLOC, 0, tamanio, NULL, NULL, 0);
}

void enviarFree(int socketReceptor, int32_t proceso, uint32_t posicion) {
	enviarOperacion(socketReceptor, proceso, FREE, posicion, 0, NULL, NULL, 0);
}

void enviarGet(int socketReceptor, int32_t proceso, uint32_t posMuse, int32_t cantBytes) {
	enviarOperacion(socketReceptor, proceso, GET, posMuse, cantBytes, NULL, NULL, 0);
}

void enviarCpy(int socketReceptor, int32_t proceso, uint32_t posDestino, void* origen, int32_t cantBytes) {
	enviarOperacion(socketReceptor, proceso, CPY, posDestino, cantBytes, origen, NULL, 0);
}

void enviarMap(int socketReceptor, int32_t proceso, char* pathArchivo, int32_t flag) {
	enviarOperacion(socketReceptor, proceso, MAP, 0, 0, NULL, pathArchivo, flag);
}

void enviarSync(int socketReceptor, int32_t proceso, uint32_t posMuse, int32_t cantBytes) {
	enviarOperacion(socketReceptor, proceso, SYNC, posMuse, cantBytes, NULL, NULL, 0);
}

void enviarUnmap(int socketReceptor, int32_t proceso, uint32_t posicion) {
	enviarOperacion(socketReceptor, proceso, UNMAP, posicion, 0, NULL, NULL, 0);
}

void enviarClose(int socketReceptor, int32_t proceso) {
	enviarOperacion(socketReceptor, proceso, CLOSE, 0, 0, NULL, NULL, 1);
}

void enviarOperacion(int socket, int32_t proceso, int32_t operacion, uint32_t posicion,
		int32_t tamanio, void* origen, char* contenido, int32_t flag) {

	int32_t desplazamiento = 0;
	int tamanioBuffer = sizeof(int32_t) * 2;

	if(posicion != 0)
		tamanioBuffer += sizeof(uint32_t);
	if(tamanio != 0)
		tamanioBuffer += sizeof(int32_t);
	if(origen != NULL)
		tamanioBuffer += tamanio;
	if(contenido != NULL)
		tamanioBuffer += strlen(contenido) + 1;
//	if(destino != 0) --> Revisar
//		tamanioBuffer += sizeof(uint32_t);
	if(flag != 0)
		tamanioBuffer += sizeof(int32_t);

	void* buffer = malloc(tamanioBuffer);

	serializarInt(buffer, proceso , &desplazamiento);
	serializarInt(buffer, operacion, &desplazamiento);

	switch(operacion) {
		case HANDSHAKE:
			serializarInt(buffer, flag, &desplazamiento);
			break;
		case MALLOC:
			serializarInt(buffer, tamanio, &desplazamiento);
			break;
		case FREE:
			serializarUint(buffer, posicion, &desplazamiento);
			break;
		case GET:
			serializarUint(buffer, posicion, &desplazamiento);
			serializarInt(buffer, tamanio, &desplazamiento);
			break;
		case CPY:
			serializarUint(buffer, posicion, &desplazamiento);
			serializarVoid(buffer, origen, tamanio, &desplazamiento);
			break;
		case MAP:
			serializarString(buffer, contenido, &desplazamiento);
			serializarInt(buffer, flag, &desplazamiento);
			break;
		case SYNC:
			serializarUint(buffer, posicion, &desplazamiento);
			serializarInt(buffer, tamanio, &desplazamiento);
			break;
		case UNMAP:
			serializarUint(buffer, posicion, &desplazamiento);
			break;
		case CLOSE:
			serializarInt(buffer, flag, &desplazamiento);
			break;
		default:
			;
	}

	enviar(socket, buffer, tamanioBuffer);
	free(buffer);

}

t_mensajeMuse* recibirOperacion(int socketEmisor) {

	t_mensajeMuse* mensajeRecibido;
	int cantidadRecibida = 0;
	int desplazamiento = 0;
	int32_t proceso, operacion;
	void* buffer = NULL;

	cantidadRecibida = recibirInt(socketEmisor, &proceso);
	cantidadRecibida += recibirInt(socketEmisor, &operacion);

	if(cantidadRecibida != sizeof(int32_t)*2)
	{
		char msj[300]; sprintf(msj,"Se recibieron %d bytes ...", cantidadRecibida);
		loggearInfo(msj);
		return NULL;
	}

	mensajeRecibido = malloc(sizeof(*mensajeRecibido)); // Ojota con los tamanios variables ... (mensajeRecibido != *mensajeRecibido)

	mensajeRecibido->idProceso = proceso;
	mensajeRecibido->tipoOperacion = operacion;

	switch(operacion) {
		case HANDSHAKE:
			recibirInt(socketEmisor, &mensajeRecibido->flag); // Podria omitirse
			break;
		case CLOSE:
			recibirInt(socketEmisor, &mensajeRecibido->flag); // Podria omitirse
			break;
		case MALLOC:
			recibirInt(socketEmisor, &mensajeRecibido->tamanio);
			break;
		case FREE:
			recibirUint(socketEmisor, &mensajeRecibido->posicionMemoria);
			break;
		case GET:
			recibirUint(socketEmisor, &mensajeRecibido->posicionMemoria);
			recibirInt(socketEmisor, &mensajeRecibido->tamanio);
			break;
		case CPY:
			recibirUint(socketEmisor, &mensajeRecibido->posicionMemoria); //destino
			recibirInt(socketEmisor, &mensajeRecibido->tamanio); //cantidad de bytes a recibir
			buffer = malloc(mensajeRecibido->tamanio);
			recibir(socketEmisor, buffer, mensajeRecibido->tamanio); //bytes a recibir
			deserializarVoid(buffer, &mensajeRecibido->origen, mensajeRecibido->tamanio, &desplazamiento); // OJO
			break;
		case MAP:
			recibirInt(socketEmisor, &mensajeRecibido->tamanio);
			buffer = malloc(mensajeRecibido->tamanio);
			recibir(socketEmisor, buffer, mensajeRecibido->tamanio);
			deserializarVoid(buffer, (void**)&mensajeRecibido->contenido, mensajeRecibido->tamanio, &desplazamiento);
			recibirInt(socketEmisor, &mensajeRecibido->flag);
			break;
		case SYNC:
			recibirUint(socketEmisor, &mensajeRecibido->posicionMemoria);
			recibirInt(socketEmisor, &mensajeRecibido->tamanio);
			break;
		case UNMAP:
			recibirUint(socketEmisor, &mensajeRecibido->posicionMemoria);
			break;
		default:
			return NULL;
	}

	return mensajeRecibido;

}
