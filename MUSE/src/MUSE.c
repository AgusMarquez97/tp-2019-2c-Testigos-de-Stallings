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
			//liberarMemoria(mensajeRecivido->idProceso); // funcion que debe liberar la memoria reservada tanto principal como swap y debe eliminar la entrada del diccionario
			enviarInt(socketRespuesta, 1);
			break;
		case MALLOC:
			msj = malloc(strlen("Se recibio una operacion ALLOC de 9999999999999999999999 bytes") + 1);
			sprintf(msj,"Se recibio una operacion ALLOC de %d bytes",mensajeRecibido->tamanio);
			loggearInfo(msj);
			free(msj);
			//resolverMalloc(mensajeRecivido->idProceso,mensajeRecivido->tamanio);
			enviarUint(socketRespuesta,1);
			break;
		case FREE:
			msj = malloc(strlen("Se recibio una operacion FREE sobre la direccion 9999999999999999999999 de memoria") + 1);
			sprintf(msj,"Se recibio una operacion FREE sobre la direccion %u de memoria",mensajeRecibido->posicionMemoria);
			loggearInfo(msj);
			free(msj);
			//liberarDireccionDeMemoria(mensajeRecivido->idProceso,mensajeRecibido->posicionMemoria);
			enviarInt(socketRespuesta, 1);
			break;
		case GET:
			msj = malloc(strlen("Se recibio una operacion GET sobre la direccion 9999999999999999999999 de 9999999999999999999999 de bytes") + 1);
			sprintf(msj,"Se recibio una operacion GET sobre la direccion %u de %d bytes",mensajeRecibido->posicionMemoria,mensajeRecibido->tamanio);
			loggearInfo(msj);
			free(msj);

			char aux[] = "Prueba";
			enviarVoid(socketRespuesta, aux, strlen(aux));
			break;
		default: //incluye el handshake
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
