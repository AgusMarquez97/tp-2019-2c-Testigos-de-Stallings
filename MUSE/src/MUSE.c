#include "MUSE.h"

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

void levantarConfig() {

	t_config * unConfig = retornarConfig(pathConfig);

	strcpy(ip,config_get_string_value(unConfig,"IP"));
	strcpy(puerto,config_get_string_value(unConfig,"LISTEN_PORT"));
	tamMemoria = config_get_int_value(unConfig,"MEMORY_SIZE");
	tamPagina = config_get_int_value(unConfig,"PAGE_SIZE");
	tamSwap = config_get_int_value(unConfig,"SWAP_SIZE");

	config_destroy(unConfig);

	loggearInfoServidor(ip,puerto);

}

void levantarMemoria() {
	dictionarioProcesos = dictionary_create();
}

void levantarServidorMUSE() {

	int socketRespuesta;

	pthread_t hiloAtendedor = 0;

	int socketServidor = levantarServidor(ip,puerto);

	while(1) {
		if((socketRespuesta = (intptr_t)aceptarConexion(socketServidor)) != -1) {
			loggearNuevaConexion(socketRespuesta);
			int* p_socket = malloc(sizeof(int));
			*p_socket = socketRespuesta;
			if((hiloAtendedor = makeDetachableThread(rutinaServidor,(void*)p_socket)) != 0) {
			}
			else {
				loggearError("Error al crear un nuevo hilo");
			}
		}
	}

}

void rutinaServidor(int* p_socket) {
	char* info;
	int socketRespuesta = *p_socket;
	free(p_socket);
	t_mensajeMuse* mensajeRecibido = recibirOperacion(socketRespuesta);
	if(mensajeRecibido == NULL) {
		loggearInfo("Handshake exitoso");
	}
	else {
		switch(mensajeRecibido->tipoOperacion) {
			case MALLOC:
				info = malloc(strlen("Se_recibió_una_operación_ALLOC_de_9999999999999999999999_bytes") + 1);
				sprintf(info, "Se recibió una operación ALLOC de %d bytes", mensajeRecibido->tamanio);
				loggearInfo(info);
				free(info);

				uint32_t retornoMalloc = procesarMalloc(mensajeRecibido->idProceso, mensajeRecibido->tamanio);

				enviarUint(socketRespuesta, retornoMalloc);
				break;
			case FREE:
				info = malloc(strlen("Se_recibió_una_operación_FREE_sobre_la_dirección_9999999999999999999999_de_memoria") + 1);
				sprintf(info, "Se recibió una operación FREE sobre la dirección %u de memoria", mensajeRecibido->posicionMemoria);
				loggearInfo(info);
				free(info);

				uint32_t retornoFree = procesarFree(mensajeRecibido->idProceso, mensajeRecibido->posicionMemoria);

				enviarInt(socketRespuesta, retornoFree);
				break;
			case GET:
				info = malloc(strlen("Se_recibió_una_operación_GET_sobre_la_dirección_9999999999999999999999_de_9999999999999999999999_bytes") + 1);
				sprintf(info, "Se recibió una operación GET sobre la dirección %u de %d bytes", mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio);
				loggearInfo(info);
				free(info);

				char* retornoGet = malloc(strlen("PRUEBA") + 1);
				retornoGet = procesarGet(mensajeRecibido->idProceso, mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio);

				enviarVoid(socketRespuesta, retornoGet, strlen(retornoGet));
				break;
			case CPY:
				info = malloc(strlen("Se_recibió_una_operación_CPY_sobre_la_dirección_9999999999999999999999_de_9999999999999999999999_bytes") + 1);
				sprintf(info, "Se recibió una operación CPY sobre la dirección %u de %d bytes", mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio);
		 		loggearInfo(info);
				free(info);

				int retornoCpy = procesarCpy(mensajeRecibido->idProceso, mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio, mensajeRecibido->contenido);

				enviarInt(socketRespuesta, retornoCpy);
				break;
			case MAP:
				info = malloc(strlen("Se_recibió_un_MAP_con_el_flag_9999999999999999999999") + 1);
				sprintf(info, "Se recibió un MAP con el flag %d", mensajeRecibido->flag);
				loggearInfo(info);
				free(info);

				uint32_t retornoMap = procesarMap(mensajeRecibido->idProceso, mensajeRecibido->contenido, mensajeRecibido->tamanio, mensajeRecibido->flag);

				enviarUint(socketRespuesta, retornoMap);
				break;
			case SYNC:
				info = malloc(strlen("Se_recibió_un_SYNC_sobre_la_dirección_de_memoria_9999999999999999999999") + 1);
				sprintf(info, "Se recibió un SYNC sobre la dirección de memoria %u", mensajeRecibido->posicionMemoria);
				loggearInfo(info);
				free(info);

				int retornoSync = procesarSync(mensajeRecibido->idProceso, mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio);

				enviarInt(socketRespuesta, retornoSync);
				break;
			case UNMAP:
				info = malloc(strlen("Se_recibió_un_UNMAP_sobre_la_dirección_9999999999999999999999") + 1);
				sprintf(info, "Se recibió un UNMAP sobre la dirección %u", mensajeRecibido->posicionMemoria);
				loggearInfo(info);
				free(info);

				int retornoUnmap = procesarUnmap(mensajeRecibido->idProceso, mensajeRecibido->posicionMemoria);

				enviarInt(socketRespuesta, retornoUnmap);
				break;

		case CLOSE:
			loggearInfo("Se recibio una operacion CLOSE");

			int retornoClose = procesarClose(mensajeRecibido->idProceso); //funcion que debe liberar la memoria reservada tanto principal como swap y debe eliminar la entrada del diccionario

			enviarInt(socketRespuesta, retornoClose);
			break;
		default: //incluye el handshake
			break;
		}
		free(mensajeRecibido);
	}

	close(socketRespuesta);

}

uint32_t procesarMalloc(int32_t idProceso, int32_t tamanio) {
	return 1;
}

uint32_t procesarFree(int32_t idProceso, uint32_t posicionMemoria) {
	return 1;
}

char* procesarGet(int32_t idProceso, uint32_t posicionMemoria, int32_t tamanio) {
	return "PRUEBA";
}

int procesarCpy(int32_t idProceso, uint32_t posicionMemoria, int32_t tamanio, void* origen) {

	char* cadena = malloc(tamanio + 1);

	memcpy(cadena, origen, tamanio);
	cadena[tamanio] = 0;
	loggearInfo(cadena);
	free(cadena);

	return 0;

}

uint32_t procesarMap(int32_t idProceso, void* contenido, int32_t tamanio, int32_t flag) {

	char* pathArchivo = malloc(tamanio + 1);

	memcpy(pathArchivo, contenido, tamanio);
	pathArchivo[tamanio] = 0;
	loggearInfo(pathArchivo);
	free(pathArchivo);

	return 1;

}

int procesarSync(int32_t idProceso, uint32_t posicionMemoria, int32_t tamanio) {
	return 0;
}

int procesarUnmap(int32_t idProceso, uint32_t posicionMemoria) {
	return 0;
}

int procesarClose(int32_t idProceso) {
	return 0;
}

void liberarVariablesGlobales() {
	destruirLog();
}










