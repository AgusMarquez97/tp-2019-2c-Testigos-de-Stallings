#include "mensajesMuse.h"

/*
 * ENVIAR DATOS MUSE
 */

void enviarClose(int socketReceptor,int32_t id_proceso)
{
	int desplazamiento = 0;
	void* buffer = malloc(sizeof(int32_t)*2);


	serializarInt(buffer, CLOSE , &desplazamiento);
	serializarInt(buffer, id_proceso , &desplazamiento);

	enviar(socketReceptor, buffer, sizeof(int32_t)*2);

	free(buffer);
}

void enviarMalloc(int socketReceptor,int32_t id_proceso, int32_t tam)
{
	int desplazamiento = 0;
	void* buffer = malloc(sizeof(int32_t)*3);


	serializarInt(buffer, MALLOC , &desplazamiento);
	serializarInt(buffer, id_proceso , &desplazamiento);
	serializarInt(buffer, tam , &desplazamiento);

	enviar(socketReceptor, buffer, sizeof(int32_t)*3);

	free(buffer);
}

void enviarFree(int socketReceptor,int32_t id_proceso, uint32_t posicion)
{
	int desplazamiento = 0;
	void* buffer = malloc(sizeof(int32_t)*2 + sizeof(uint32_t));


	serializarInt(buffer, FREE , &desplazamiento);
	serializarInt(buffer, id_proceso , &desplazamiento);
	serializarUint(buffer, posicion , &desplazamiento);

	enviar(socketReceptor, buffer, sizeof(int32_t)*2 + sizeof(uint32_t));

	free(buffer);
}

void enviarUnmap(int socketReceptor,int32_t id_proceso, uint32_t posicion)
{
	int desplazamiento = 0;
	void* buffer = malloc(sizeof(int32_t)*2 + sizeof(uint32_t));


	serializarInt(buffer, UNMAP , &desplazamiento);
	serializarInt(buffer, id_proceso , &desplazamiento);
	serializarUint(buffer, posicion , &desplazamiento);

	enviar(socketReceptor, buffer, sizeof(int32_t)*2 + sizeof(uint32_t));

	free(buffer);
}


void enviarGet(int socketReceptor,int32_t id_proceso, uint32_t posicionMuse, int32_t cantidadBytes)
{
	int desplazamiento = 0;
	void* buffer = malloc(sizeof(int32_t)*3 + sizeof(uint32_t));


	serializarInt(buffer, GET , &desplazamiento);
	serializarInt(buffer, id_proceso , &desplazamiento);
	serializarUint(buffer, posicionMuse , &desplazamiento);
	serializarInt(buffer, cantidadBytes , &desplazamiento);

	enviar(socketReceptor, buffer, sizeof(int32_t)*3 + sizeof(uint32_t));

	free(buffer);
}

void enviarSync(int socketReceptor,int32_t id_proceso, uint32_t posicionMuse, int32_t cantidadBytes)
{
	int desplazamiento = 0;
	void* buffer = malloc(sizeof(int32_t)*3 + sizeof(uint32_t));


	serializarInt(buffer, SYNC , &desplazamiento);
	serializarInt(buffer, id_proceso , &desplazamiento);
	serializarUint(buffer, posicionMuse , &desplazamiento);
	serializarInt(buffer, cantidadBytes , &desplazamiento);

	enviar(socketReceptor, buffer, sizeof(int32_t)*3 + sizeof(uint32_t));

	free(buffer);
}



void enviarCpy(int socketReceptor,int32_t id_proceso,uint32_t posicionDestino, void * origen, int32_t cantidadBytes)
{
	int desplazamiento = 0;
	void* buffer = malloc(sizeof(int32_t)*3 + sizeof(uint32_t) + cantidadBytes);

	serializarInt(buffer, CPY , &desplazamiento);
	serializarInt(buffer, id_proceso , &desplazamiento);
	serializarUint(buffer,posicionDestino,&desplazamiento);
	serializarVoid(buffer,origen,cantidadBytes,&desplazamiento);

	enviar(socketReceptor, buffer, sizeof(int32_t)*3 + sizeof(uint32_t) + cantidadBytes);

	free(buffer);
}

void enviarMap(int socketReceptor,int32_t id_proceso, char * contenidoArchivo, int32_t flag)
{
	int desplazamiento = 0;
	void* buffer = malloc(sizeof(int32_t)*4 + strlen(contenidoArchivo) + 1);

	serializarInt(buffer, MAP , &desplazamiento);
	serializarInt(buffer, id_proceso , &desplazamiento);
	serializarString(buffer, contenidoArchivo, &desplazamiento);
	serializarInt(buffer, flag , &desplazamiento);

	enviar(socketReceptor, buffer, sizeof(int32_t)*4 + strlen(contenidoArchivo) + 1);

	free(buffer);
}

void enviarHandshake(int socketReceptor,int32_t id_proceso)
{
	int desplazamiento = 0;
	void* buffer = malloc(sizeof(int32_t)*2);


	serializarInt(buffer, HANDSHAKE, &desplazamiento);
	serializarInt(buffer, id_proceso , &desplazamiento);

	enviar(socketReceptor, buffer, sizeof(int32_t)*2);

	free(buffer);
}




mensajeMuse * recibirRequestMuse(int socketEmisor)
{
		mensajeMuse * mensajeRetorno;

		int cantidadRecibida;
		int desplazamiento = 0;

		int32_t tipoOperacion;
		int32_t idProceso;

		void * buffer = NULL;

		cantidadRecibida = recibirInt(socketEmisor, &tipoOperacion);
		cantidadRecibida += recibirInt(socketEmisor, &idProceso);

		if(cantidadRecibida != sizeof(int32_t)*2)
			return NULL;

		mensajeRetorno = malloc(sizeof(mensajeRetorno));

		mensajeRetorno->tipoOperacion =  tipoOperacion;
		mensajeRetorno->idProceso =  idProceso;

		switch(tipoOperacion)
		{
		case MALLOC:
			recibirInt(socketEmisor,&mensajeRetorno->tamanio);
			break;
		case FREE:
			recibirUint(socketEmisor,&mensajeRetorno->posicionMemoria);
			break;
		case GET:
			recibirUint(socketEmisor,&mensajeRetorno->posicionMemoria);
			recibirInt(socketEmisor,&mensajeRetorno->tamanio);
			break;
		case CPY:
			recibirUint(socketEmisor, &mensajeRetorno->posicionMemoria); // donde pegar los bytes que voy a recibir
			recibirInt(socketEmisor,&mensajeRetorno->tamanio); // cantidad de bytes a recibir
			buffer = malloc(mensajeRetorno->tamanio);
			recibir(socketEmisor,buffer,mensajeRetorno->tamanio); // los bytes a recibir
			deserializarVoid(buffer, &mensajeRetorno->contenido,mensajeRetorno->tamanio,&desplazamiento);
			break;
		case MAP:
			recibirInt(socketEmisor,&mensajeRetorno->tamanio);
			buffer = malloc(mensajeRetorno->tamanio);
			recibir(socketEmisor,buffer,mensajeRetorno->tamanio);
			deserializarVoid(buffer, &mensajeRetorno->contenido,mensajeRetorno->tamanio,&desplazamiento);
			recibirInt(socketEmisor,&mensajeRetorno->flag);
			break;
		case SYNC:
			recibirUint(socketEmisor,&mensajeRetorno->posicionMemoria);
			recibirInt(socketEmisor,&mensajeRetorno->tamanio);
			break;
		case UNMAP:
			recibirUint(socketEmisor,&mensajeRetorno->posicionMemoria);
			break;
		case (CLOSE || HANDSHAKE):
			break;
		default:
			return NULL;
		}

	return mensajeRetorno;
}
