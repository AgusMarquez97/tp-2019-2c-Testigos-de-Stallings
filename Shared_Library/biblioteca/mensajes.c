#include "mensajes.h"

/*
 * ENVIAR DATOS MUSE
 */

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



void enviarCpy(int socketReceptor,int32_t id_proceso, void * origen, int32_t cantidadBytes)
{
	int desplazamiento = 0;
	void* buffer = malloc(sizeof(int32_t)*3 + cantidadBytes);

	serializarInt(buffer, CPY , &desplazamiento);
	serializarInt(buffer, id_proceso , &desplazamiento);
	serializarVoid(buffer,origen,cantidadBytes,&desplazamiento);

	enviar(socketReceptor, buffer, sizeof(int32_t)*3 + cantidadBytes);

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
			recibirInt(socketEmisor,&mensajeRetorno->tamanio);
			buffer = malloc(mensajeRetorno->tamanio);
			recibir(socketEmisor,buffer,mensajeRetorno->tamanio);
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
		case CLOSE:
			break;
		default:
			return NULL;
		}

	return mensajeRetorno;
}




//ENVIAR DATOS PRIMITIVOS

void enviarUint(int socketReceptor, uint32_t entero){

	int desplazamiento = 0;
	void* buffer = malloc(sizeof(uint32_t));

	serializarUint(buffer, entero, &desplazamiento);
	enviar(socketReceptor, buffer, sizeof(uint32_t));

	free(buffer);

}

void enviarInt(int socketReceptor, int32_t entero){

	int desplazamiento = 0;
	void* buffer = malloc(sizeof(int32_t));

	serializarInt(buffer, entero, &desplazamiento);
	enviar(socketReceptor, buffer, sizeof(int32_t));

	free(buffer);

}

void enviarChar(int socketReceptor, char caracter){

	int desplazamiento = 0;
	void* buffer = malloc(sizeof(char));

	serializarChar(buffer, caracter, &desplazamiento);
	enviar(socketReceptor, buffer, sizeof(char));

	free(buffer);

}

void enviarString(int socketReceptor, char* cadena){

	int desplazamiento = 0;

	if(cadena == NULL)
	{
		cadena = strdup("VOID");
	}

	int32_t tamanioCadena = sizeof(int32_t) + strlen(cadena) + 1;
	int32_t tamanioBuffer = sizeof(int32_t) + tamanioCadena;

	void* buffer = malloc(tamanioBuffer);

	serializarInt(buffer,tamanioCadena,&desplazamiento);

	serializarString(buffer, cadena, &desplazamiento);

	enviar(socketReceptor, buffer, tamanioBuffer);

	free(buffer);
	free(cadena);
}



//RECIBIR DATOS PRIMITIVOS
//revisar uso
int recibirInt(int socketEmisor, int32_t* entero){

	int desplazamiento = 0;
	void* buffer = malloc(sizeof(int32_t));

	int cantidadRecibida = recibir(socketEmisor, buffer, sizeof(int32_t)); //Crear / acomodar esta funcion
	deserializarInt(buffer, entero, &desplazamiento);

	free(buffer);

	return cantidadRecibida;
}

int recibirUint(int socketEmisor, uint32_t* entero){

	int desplazamiento = 0;
	void* buffer = malloc(sizeof(uint32_t));

	int cantidadRecibida = recibir(socketEmisor, buffer, sizeof(uint32_t));
	deserializarUint(buffer, entero, &desplazamiento);

	free(buffer);

	return cantidadRecibida;
}

int recibirChar(int socketEmisor, char* caracter){

	int desplazamiento = 0;
	void* buffer = malloc(sizeof(char));

	int cantidadRecibida = recibir(socketEmisor, buffer, sizeof(char));
	deserializarChar(buffer, caracter, &desplazamiento);

	free(buffer);
	return cantidadRecibida;
}
int recibirString(int socketEmisor, char** cadena)
{
		int cantidadRecibida;
		int desplazamiento = 0;
		int tamBuffer = 0;

		void * buffer = malloc(sizeof(int32_t)); //Primero se reserva el tamanio de lo que se va a recibir

		cantidadRecibida = recibir(socketEmisor,buffer,sizeof(int32_t));

		deserializarInt(buffer,&tamBuffer,&desplazamiento); //Se recibe el tam del buffer!!

		buffer = realloc(buffer,tamBuffer);

		cantidadRecibida += recibir(socketEmisor,buffer,tamBuffer); //Se recibe el buffer

		desplazamiento = 0; //Me paro en el inicio del nuevo buffer

		deserializarString(buffer, cadena, &desplazamiento);

		char * log = malloc(100);
		strcpy(log,"Se recibio {");
		strcat(log,*cadena);
		strcat(log,"}");
		loggearInfo(log);

		free(*cadena);
		free(log);
		free(buffer);

		return cantidadRecibida;
}






//RECIBIR QUERY

/*
 * void enviarQuery(int socketReceptor, query* myQuery)
{
	int32_t nroViajar = -1;

	if(myQuery == NULL)
	{
		enviarInt(socketReceptor,nroViajar);
	}
	else
	{
		switch(myQuery->requestType) {
			case SELECT:
				enviarSelect(socketReceptor, myQuery->tabla, myQuery->key);
				break;
			case INSERT:
				enviarInsert(socketReceptor, myQuery->tabla, myQuery->key, myQuery->value, myQuery->timestamp);
				break;
			case CREATE:
				enviarCreate(socketReceptor, myQuery->tabla, myQuery->consistencyType, myQuery->cantParticiones, myQuery->compactationTime);
				break;
			case DESCRIBE:
				if(myQuery->tabla) // !=  NULL
				{
				enviarDescribe(socketReceptor,myQuery->tabla);
				}
				else
				{
				enviarRequest(socketReceptor,DESCRIBE);
				}
				break;
			case DROP:
				enviarDrop(socketReceptor,myQuery->tabla);
				break;
			case JOURNAL:
				enviarRequest(socketReceptor,JOURNAL);
				break;
			case HANDSHAKE:
				enviarRequest(socketReceptor,HANDSHAKE);
				break;
			case GOSSIP_KERNEL:
				enviarRequestCorta(socketReceptor,myQuery->tabla,GOSSIP_KERNEL);
				break;
			case GOSSIP:
				enviarRequestCorta(socketReceptor,myQuery->tabla,GOSSIP);
				break;
			default:
				loggearError("Request no valido");
				break;
		}
	}
}
 */

/*
int recibirQuery(int socketEmisor, query ** myQuery) {

	int cantidadRecibida;
	int desplazamiento = 0;

	int32_t tamanioQuery = 0;
	int32_t tipoQuery;
	int32_t tamanioBuffer = 0;
	void * buffer;

	cantidadRecibida = recibirInt(socketEmisor, &tamanioQuery); //Se recibe el tam del buffer => asd


	if(cantidadRecibida == 0)
	{
		return 0;
	}
	cantidadRecibida += recibirInt(socketEmisor, &tipoQuery);

	if(tipoQuery == -1)
	{
		return -1;
	}

	tamanioBuffer = tamanioQuery - sizeof(int32_t);

	if(tamanioBuffer > 0)
	{
	buffer = malloc(tamanioBuffer); //Se le resta el tamanio del tipo de query
	cantidadRecibida += recibir(socketEmisor,buffer,tamanioBuffer);
	}

	*myQuery = malloc(sizeof(query) + tamanioQuery);
	memset(*myQuery,0,sizeof(query) + tamanioQuery);

	switch(tipoQuery) {
		case SELECT:
		((*myQuery))->requestType = SELECT;
		deserializarSelect(&(*myQuery)->tabla, &(*myQuery)->key, buffer, &desplazamiento);
		loggearSelect((*myQuery)->tabla,(*myQuery)->key);
		break;
		case INSERT:
		(*myQuery)->requestType = INSERT;
		deserializarInsert(&(*myQuery)->tabla, &(*myQuery)->key, &(*myQuery)->value, &(*myQuery)->timestamp, buffer, &desplazamiento);
		loggearInsert((*myQuery)->tabla,(*myQuery)->key,(*myQuery)->value,(*myQuery)->timestamp);
		break;
		case CREATE:
		((*myQuery))->requestType = CREATE;
		deserializarCreate(&(*myQuery)->tabla, &(*myQuery)->consistencyType, &(*myQuery)->cantParticiones, &(*myQuery)->compactationTime, buffer, &desplazamiento);
		//loggearCreate((*myQuery)->tabla,(*myQuery)->consistencyType, (*myQuery)->cantParticiones, (*myQuery)->compactationTime);
		break;
		case DESCRIBE:
		((*myQuery))->requestType = DESCRIBE;
		if(tamanioQuery > sizeof(int32_t))
		{
		deserializarDescribe(&(*myQuery)->tabla, buffer, &desplazamiento);
		loggearInfo3Mensajes("Se recibio la siguiente query: {DESCRIBE ",(*myQuery)->tabla,"}");
		}else
		{
			*myQuery = realloc(*myQuery,sizeof(query));
			(*myQuery)->tabla = NULL;
			loggearInfo("Se recibio la siguiente query: {DESCRIBE}");
		}
			break;
		case DROP:
		((*myQuery))->requestType = DROP;
		deserializarDrop(&(*myQuery)->tabla, buffer, &desplazamiento);
		loggearInfo3Mensajes("Se recibio la siguiente query: {DROP ",(*myQuery)->tabla,"}");
			break;
		case JOURNAL:
		((*myQuery))->requestType = JOURNAL;
		loggearInfo("Se recibio un request Journal");
			break;
		case HANDSHAKE:
		((*myQuery))->requestType = HANDSHAKE;
		loggearInfo("Se realiza un Handshake");
		break;
		case RUN:
		((*myQuery))->requestType = RUN;
		loggearInfo("Se obtiene la consistencia de la memoria");
			break;
		case GOSSIP:
			deserializarRequestCorta(&(*myQuery)->tabla, buffer, &desplazamiento);
			break;
		case GOSSIP_KERNEL:
			deserializarRequestCorta(&(*myQuery)->tabla, buffer, &desplazamiento);
			break;
		default:
			return -1;
	}
	if(tamanioBuffer > 0)
	{
	free(buffer);
	}
	return cantidadRecibida;
}
*/







