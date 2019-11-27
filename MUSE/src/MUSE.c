#include "MuseOperaciones.h"

int main(void) {

	remove("Linuse.log");
	iniciarLog("MUSE");
	loggearInfo("Se inicia el proceso MUSE...");
	levantarConfig();
	levantarMemoria();
	inicializarSemaforos();
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
	diccionarioProcesos = dictionary_create();
	memoria = malloc(tamMemoria);
	crearMemoriaSwap();

	levantarMarcos(&marcosMemoriaPrincipal,tamMemoria,&cantidadMarcosMemoriaPrincipal);
	levantarMarcos(&marcosMemoriaSwap,tamSwap,&cantidadMarcosMemoriaVirtual);


}

void levantarMarcos(t_bitarray ** unBitArray, int tamanio, int * cantidadMarcos)
{
		*cantidadMarcos = obtenerCantidadMarcos(tamPagina,tamanio);
		char * bitmap = calloc(1,*cantidadMarcos); // Cuando se libera??

		*unBitArray = bitarray_create_with_mode(bitmap, (*cantidadMarcos)/8, LSB_FIRST); // Array de bits para consultar marcos libres

		for(int i = 0; i<(*cantidadMarcos);i++)
		{
		bitarray_clean_bit(*unBitArray,i); //seteo todos los bits en 0 (al principio todos los marcos están libres)
		}
}

void crearMemoriaSwap()
{
	remove("Memoria Swap"); // Analizar si la MS debe ser persistida
	FILE * f_MS =  txt_open_for_append("Memoria Swap");
	txt_close_file(f_MS);
}

void inicializarSemaforos()
{
	pthread_mutex_init(&mutex_marcos_libres, NULL);
	pthread_mutex_init(&mutex_diccionario, NULL);
	pthread_mutex_init(&mutex_memoria, NULL);

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
	int valorRetorno;
	int socketRespuesta = *p_socket;
	char id_proceso[30];
	free(p_socket);
	t_mensajeMuse* mensajeRecibido = recibirOperacion(socketRespuesta);

	sprintf(id_proceso,"%d",mensajeRecibido->idProceso);

	if(mensajeRecibido == NULL) {
		loggearInfo("Mensaje no reconocido");
	}
	else {
		switch(mensajeRecibido->tipoOperacion) {
			case HANDSHAKE:
			valorRetorno = procesarHandshake(id_proceso);
			enviarInt(socketRespuesta,valorRetorno);
			break;
			case MALLOC:
				info = malloc(strlen("Se_recibió_una_operación_ALLOC_de_9999999999999999999999_bytes") + 1);
				sprintf(info, "Se recibió una operación ALLOC de %d bytes", mensajeRecibido->tamanio);
				loggearInfo(info);
				free(info);
				uint32_t retornoMalloc = procesarMalloc(id_proceso, mensajeRecibido->tamanio);
				enviarUint(socketRespuesta, retornoMalloc);
				break;
			case FREE:
				info = malloc(strlen("Se_recibió_una_operación_FREE_sobre_la_dirección_9999999999999999999999_de_memoria") + 1);
				sprintf(info, "Se recibió una operación FREE sobre la dirección %u de memoria", mensajeRecibido->posicionMemoria);
				loggearInfo(info);
				free(info);

				int32_t retornoFree = procesarFree(id_proceso, mensajeRecibido->posicionMemoria);

				enviarInt(socketRespuesta, retornoFree);
				break;
			case GET:
				info = malloc(strlen("Se_recibió_una_operación_GET_sobre_la_dirección_9999999999999999999999_de_9999999999999999999999_bytes") + 1);
				sprintf(info, "Se recibió una operación GET sobre la dirección %u de %d bytes", mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio);
				loggearInfo(info);
				free(info);

				void* retornoGet = malloc(mensajeRecibido->tamanio);
				retornoGet = procesarGet(id_proceso, mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio);
				enviarVoid(socketRespuesta, retornoGet, mensajeRecibido->tamanio);

				free(retornoGet);
				break;
			case CPY:
				info = malloc(strlen("Se_recibió_una_operación_CPY_sobre_la_dirección_9999999999999999999999_de_9999999999999999999999_bytes") + 1);
				sprintf(info, "Se recibió una operación CPY sobre la dirección %u de %d bytes", mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio);
		 		loggearInfo(info);
				free(info);

				int retornoCpy = procesarCpy(id_proceso, mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio, mensajeRecibido->contenido);

				enviarInt(socketRespuesta, retornoCpy);
				break;
			case MAP:
				info = malloc(strlen("Se_recibió_un_MAP_con_el_flag_9999999999999999999999") + 1);
				sprintf(info, "Se recibió un MAP con el flag %d", mensajeRecibido->flag);
				loggearInfo(info);
				free(info);

				uint32_t retornoMap = procesarMap(id_proceso, mensajeRecibido->contenido, mensajeRecibido->tamanio, mensajeRecibido->flag);

				enviarUint(socketRespuesta, retornoMap);
				break;
			case SYNC:
				info = malloc(strlen("Se_recibió_un_SYNC_sobre_la_dirección_de_memoria_9999999999999999999999") + 1);
				sprintf(info, "Se recibió un SYNC sobre la dirección de memoria %u", mensajeRecibido->posicionMemoria);
				loggearInfo(info);
				free(info);

				int retornoSync = procesarSync(id_proceso, mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio);

				enviarInt(socketRespuesta, retornoSync);
				break;
			case UNMAP:
				info = malloc(strlen("Se_recibió_un_UNMAP_sobre_la_dirección_9999999999999999999999") + 1);
				sprintf(info, "Se recibió un UNMAP sobre la dirección %u", mensajeRecibido->posicionMemoria);
				loggearInfo(info);
				free(info);

				int retornoUnmap = procesarUnmap(id_proceso, mensajeRecibido->posicionMemoria);

				enviarInt(socketRespuesta, retornoUnmap);
				break;

			case CLOSE:
			loggearInfo("Se recibio una operacion CLOSE");

			int retornoClose = procesarClose(id_proceso); //funcion que debe liberar la memoria reservada tanto principal como swap y debe eliminar la entrada del diccionario

			enviarInt(socketRespuesta, retornoClose);
			break;
		default:
			break;
		}
		free(mensajeRecibido);
	}

	close(socketRespuesta);

}



void liberarVariablesGlobales() {
	destruirLog();
	free(memoria);
}









