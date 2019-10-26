/*
 ============================================================================
 Name        : MUSE.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "MUSE.h"

void levantarServidorMUSE()
{
	int socketRespuesta;

	pthread_t hiloAtendedor = 0;

	int socketServidor = levantarServidor(ip,puerto);

	while(1)
	{
		if((socketRespuesta = (intptr_t)aceptarConexion(socketServidor)) != -1)
		{
			loggearNuevaConexion(socketRespuesta);
			int * p_socket = malloc(sizeof(int));
			*p_socket = socketRespuesta;
			if((hiloAtendedor = makeDetachableThread(rutinaServidor,(void*)p_socket)) != 0)
			{
			}
			else
			{
				loggearError("Error al crear un nuevo hilo");
			}
		}
	}

}

void rutinaServidor(int * p_socket)
{
	char * msj;
	int socketRespuesta = *p_socket;
	free(p_socket);
	t_mensajeMuse * mensajeRecibido = recibirOperacion(socketRespuesta);
	if(mensajeRecibido == NULL)
		loggearInfo("Handshake exitoso");
	else
	{
		switch(mensajeRecibido->tipoOperacion)
		{
		case CLOSE:
			loggearInfo("Se recibio una operacion CLOSE");
			//procesarClose(mensajeRecivido->idProceso); // funcion que debe liberar la memoria reservada tanto principal como swap y debe eliminar la entrada del diccionario
			enviarInt(socketRespuesta, 1);
			break;
		case MALLOC:
			msj = malloc(strlen("Se recibio una operacion ALLOC de 9999999999999999999999 bytes") + 1);
			sprintf(msj,"Se recibio una operacion ALLOC de %d bytes",mensajeRecibido->tamanio);
			loggearInfo(msj);
			free(msj);
			//procesarMalloc(mensajeRecivido->idProceso,mensajeRecivido->tamanio);
			enviarUint(socketRespuesta,1);
			break;
		case FREE:
			msj = malloc(strlen("Se recibio una operacion FREE sobre la direccion 9999999999999999999999 de memoria") + 1);
			sprintf(msj,"Se recibio una operacion FREE sobre la direccion %u de memoria",mensajeRecibido->posicionMemoria);
			loggearInfo(msj);
			free(msj);
			//procesarFree(mensajeRecivido->idProceso,mensajeRecibido->posicionMemoria);
			enviarInt(socketRespuesta, 1);
			break;
		case GET:
			msj = malloc(strlen("Se recibio una operacion GET sobre la direccion 9999999999999999999999 de 9999999999999999999999 de bytes") + 1);
			sprintf(msj,"Se recibio una operacion GET sobre la direccion %u de %d bytes",mensajeRecibido->posicionMemoria,mensajeRecibido->tamanio);
			loggearInfo(msj);
			free(msj);
			//procesarGet(mensajeRecivido->idProceso,mensajeRecibido->posicionMemoria,mensajeRecibido->tamanio);
			char aux[] = "Prueba";
			enviarVoid(socketRespuesta, aux, strlen(aux));
			break;
		case CPY:
			msj = malloc(strlen("Se recibio una operacion CPY sobre la direccion 9999999999999999999999 de 9999999999999999999999 de bytes") + 1 + mensajeRecibido->tamanio);
			sprintf(msj,"Se recibio una operacion CPY sobre la direccion %u de %d bytes.",mensajeRecibido->posicionMemoria,mensajeRecibido->tamanio);
	 		loggearInfo(msj);
			free(msj);

			char * cadena_ejemplo = malloc(mensajeRecibido->tamanio + 1);

			memcpy(cadena_ejemplo,mensajeRecibido->origen,mensajeRecibido->tamanio);
			cadena_ejemplo[mensajeRecibido->tamanio] = 0;

			loggearInfo(cadena_ejemplo);

			free(cadena_ejemplo);

			//procesarCpy(mensajeRecivido->idProceso,mensajeRecibido->posicionMemoria,mensajeRecibido->tamanio);

			enviarInt(socketRespuesta,1);
			break;
		case MAP:
			msj = malloc(strlen("Se recibio un MAP con el flag 9999999999999999999999 del archivo ubicado en ") + 1);
			sprintf(msj, "Se recibio un MAP con el flag %d del archivo ubicado en ", mensajeRecibido->flag);
			loggearInfo(msj);
			free(msj);

			char* pathArchivo = malloc(mensajeRecibido->tamanio + 1);

			memcpy(pathArchivo, mensajeRecibido->contenido, mensajeRecibido->tamanio);
			pathArchivo[mensajeRecibido->tamanio] = 0;
			loggearInfo(pathArchivo);
			free(pathArchivo);

			//procesarMap(mensajeRecibido->idProceso, mensajeRecibido->contenido, mensajeRecibido->tamanio, mensajeRecibido->flag);

			enviarUint(socketRespuesta, 1);
			break;
		case SYNC:
			msj = malloc(strlen("Se recibio un SYNC sobre la dirección de memoria 9999999999999999999999") + 1);
			sprintf(msj, "Se recibió un SYNC sobre la dirección de memoria %u", mensajeRecibido->posicionMemoria);
			loggearInfo(msj);
			free(msj);

			//procesarSync(mensajeRecibido->idProceso, mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio);

			enviarInt(socketRespuesta, 0);
			break;
		case UNMAP:
			msj = malloc(strlen("Se recibió un UNMAP sobre la dirección 9999999999999999999999") + 1);
			sprintf(msj, "Se recibió un UNMAP sobre la dirección %u", mensajeRecibido->posicionMemoria);
			loggearInfo(msj);
			free(msj);

			//procesarUnmap(mensajeRecibido->idProceso, mensajeRecibido->posicionMemoria);

			enviarInt(socketRespuesta, 0);
			break;
		default:
			//incluye el handshake
			break;
		}
		free(mensajeRecibido);

	}
	close(socketRespuesta);
}

void liberarVariablesGlobales()
{
	destruirLog();
}

void levantarConfig()
{
	t_config * unConfig = retornarConfig(pathConfig);

	strcpy(ip,config_get_string_value(unConfig,"IP"));
	strcpy(puerto,config_get_string_value(unConfig,"LISTEN_PORT"));
	tamMemoria = config_get_int_value(unConfig,"MEMORY_SIZE");
	tamPagina = config_get_int_value(unConfig,"PAGE_SIZE");
	tamSwap = config_get_int_value(unConfig,"SWAP_SIZE");

	config_destroy(unConfig);

	loggearInfoServidor(ip,puerto);
}

void levantarMemoria()
{
	dictionarioProcesos = dictionary_create();

}

int main(void) {
	remove("Linuse.log");
	iniciarLog("MUSE");
	loggearInfo("Se inicia el proceso MUSE...");
	levantarConfig();
	levantarMemoria();
	levantarServidorMUSE();
	liberarVariablesGlobales();
	return EXIT_SUCCESS;
}
