#include "mensajesMuse.h"

void enviarHandshake(int socketReceptor, int32_t proceso) {
	enviarOperacion(socketReceptor, proceso, HANDSHAKE, 0, 0, NULL, 1, NULL);
}

void enviarMalloc(int socketReceptor, int32_t proceso, int32_t tamanio) {
	enviarOperacion(socketReceptor, proceso, MALLOC, 0, tamanio, NULL, 0, NULL);
}

void enviarFree(int socketReceptor, int32_t proceso, uint32_t posicion) {
	enviarOperacion(socketReceptor, proceso, FREE, posicion, 0, NULL, 0, NULL);
}

void enviarGet(int socketReceptor, int32_t proceso, uint32_t posMuse, int32_t cantBytes) {
	enviarOperacion(socketReceptor, proceso, GET, posMuse, cantBytes, NULL, 0, NULL);
}

void enviarCpy(int socketReceptor, int32_t proceso, uint32_t posDestino, void* origen, int32_t cantBytes) {
	enviarOperacion(socketReceptor, proceso, CPY, posDestino, cantBytes, origen, 0, NULL);
}

void enviarMap(int socketReceptor, int32_t proceso, int32_t tamanio, int32_t flag, char* pathArchivo) {
	enviarOperacion(socketReceptor, proceso, MAP, 0, tamanio, NULL, flag, pathArchivo);
}

void enviarSync(int socketReceptor, int32_t proceso, uint32_t posMuse, int32_t cantBytes) {
	enviarOperacion(socketReceptor, proceso, SYNC, posMuse, cantBytes, NULL, 0, NULL);
}

void enviarUnmap(int socketReceptor, int32_t proceso, uint32_t posicion) {
	enviarOperacion(socketReceptor, proceso, UNMAP, posicion, 0, NULL, 0, NULL);
}

void enviarClose(int socketReceptor, int32_t proceso) {
	enviarOperacion(socketReceptor, proceso, CLOSE, 0, 0, NULL, 1, NULL);
}

void enviarOperacion(int socket, int32_t proceso, int32_t operacion, uint32_t posicion,
		int32_t tamanio, void* contenido, int32_t flag, char* pathArchivo) {

	int32_t desplazamiento = 0;
	int tamanioBuffer = sizeof(int32_t) * 2;

	if(posicion != 0 ||(operacion == CPY||operacion==GET||operacion==SYNC||operacion==UNMAP))
		tamanioBuffer += sizeof(uint32_t);
	if(tamanio != 0)
		tamanioBuffer += sizeof(int32_t);
	if(contenido != NULL)
		tamanioBuffer += tamanio;
	if(flag != 0)
		tamanioBuffer += sizeof(int32_t);
	if(pathArchivo != NULL)
		tamanioBuffer += strlen(pathArchivo)+1+sizeof(int32_t);

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
			serializarVoid(buffer, contenido, tamanio, &desplazamiento);
			break;
		case MAP:
			serializarInt(buffer, tamanio, &desplazamiento); // tamanio a leer del archivo
			serializarVoid(buffer, pathArchivo,strlen((char*)pathArchivo)+1, &desplazamiento); // longitud y nombre del archivo. VALIDAR CASTEO
			serializarInt(buffer, flag, &desplazamiento); // flag de si es o no compartido
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
			break;
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
			deserializarVoid(buffer, &mensajeRecibido->contenido, mensajeRecibido->tamanio, &desplazamiento); // OJO
			free(buffer);
			break;
		case MAP:
			recibirInt(socketEmisor, &mensajeRecibido->tamanio); // Este tamanio es el tam que se quiere leer!
			recibirInt(socketEmisor, &mensajeRecibido->flag); // Se usa el flag como variable auxiliar!
			buffer = malloc(mensajeRecibido->flag);
			recibir(socketEmisor, buffer, mensajeRecibido->flag); // recibe la candena del nombre del path y lo guarda en el buffer
			deserializarVoid(buffer, &mensajeRecibido->contenido, mensajeRecibido->flag, &desplazamiento);
			recibirInt(socketEmisor, &mensajeRecibido->flag); // Guarda el valor posta del flag
			free(buffer);
			break;
		case SYNC:
			recibirUint(socketEmisor, &mensajeRecibido->posicionMemoria);
			recibirInt(socketEmisor, &mensajeRecibido->tamanio);
			break;
		case UNMAP:
			recibirUint(socketEmisor, &mensajeRecibido->posicionMemoria);
			break;
		case CLOSE:
			recibirInt(socketEmisor, &mensajeRecibido->flag); // Podria omitirse
			break;
		default:
			return NULL;
	}

	return mensajeRecibido;

}
