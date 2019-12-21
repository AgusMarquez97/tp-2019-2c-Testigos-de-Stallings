#include "MuseOperaciones.h"

int main(void) {

	//signal(SIGINT,salirFuncion); // si rompe con varios procesos => comentar
	remove("Linuse.log");
	iniciarLog("MUSE");
	loggearInfo("Se inicia el proceso MUSE...");
	levantarConfig();
	levantarMemoria();
	inicializarSemaforos();
	levantarServidorMUSE();
	return EXIT_SUCCESS;

}

void levantarConfig() {

	t_config * unConfig = retornarConfig(pathConfig);

	strcpy(ip,config_get_string_value(unConfig, "IP"));
	strcpy(puerto,config_get_string_value(unConfig, "LISTEN_PORT"));
	tamMemoria = config_get_int_value(unConfig, "MEMORY_SIZE");
	tamPagina = config_get_int_value(unConfig, "PAGE_SIZE");
	tamSwap = config_get_int_value(unConfig, "SWAP_SIZE");

	config_destroy(unConfig);

	loggearInfoServidor(ip, puerto);

}

void levantarMemoria() {

	diccionarioProcesos = dictionary_create();
	listaArchivosCompartidos = list_create();
	listaPaginasClockModificado = list_create();
	ptrAlgoritmoPaginaSiguiente = 0;
	memoria = malloc(tamMemoria);
	crearMemoriaSwap();

	levantarMarcos(&marcosMemoriaPrincipal, tamMemoria, &cantidadMarcosMemoriaPrincipal);
	levantarMarcos(&marcosMemoriaSwap, tamSwap, &cantidadMarcosMemoriaVirtual);

}

void levantarMarcos(t_bitarray** unBitArray, int tamanio, int* cantidadMarcos) {

	*cantidadMarcos = obtenerCantidadMarcos(tamPagina, tamanio);
	char* bitmap = calloc(1, *cantidadMarcos);
	// Array de bits para consultar marcos libres
	*unBitArray = bitarray_create_with_mode(bitmap, (*cantidadMarcos)/8, LSB_FIRST);

	for(int i = 0; i<(*cantidadMarcos); i++) {
		bitarray_clean_bit(*unBitArray, i);
	}

}

void crearMemoriaSwap() {

	remove("Memoria Swap");
	FILE* f_MS =  txt_open_for_append("Memoria Swap");
	int fd_num = fileno(f_MS);
	ftruncate(fd_num,tamSwap);
	txt_close_file(f_MS);

}

void inicializarSemaforos() {

	pthread_mutex_init(&mutex_marcos_swap_libres, NULL);
	pthread_mutex_init(&mutex_marcos_libres, NULL);
	pthread_mutex_init(&mutex_diccionario, NULL);
	pthread_mutex_init(&mutex_memoria, NULL);
	pthread_mutex_init(&mutex_lista_archivos, NULL);
	pthread_mutex_init(&mutex_algoritmo_reemplazo, NULL);
	pthread_mutex_init(&mutex_lista_paginas, NULL);
	pthread_mutex_init(&mutex_segmento, NULL);
	pthread_mutex_init(&mutex_lock_operaciones, NULL);

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

			if((hiloAtendedor = makeDetachableThread(rutinaServidor, (void*)p_socket)) != 0) {

			} else {
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

	sprintf(id_proceso,"%d", mensajeRecibido->idProceso);

	if(mensajeRecibido == NULL) {
		loggearWarning("Operación Desconocida");
	} else {
		//pthread_mutex_lock(&mutex_lock_operaciones);
		switch(mensajeRecibido->tipoOperacion) {

			case HANDSHAKE:
				valorRetorno = procesarHandshake(id_proceso);
				enviarInt(socketRespuesta,valorRetorno);

				break;
			case MALLOC:
				info = malloc(strlen("[pid|%s]-> ALLOC de 999999999999999 bytes") + 1);
				sprintf(info, "[pid|%s]-> ALLOC de %d bytes", id_proceso, mensajeRecibido->tamanio);
				loggearInfo(info);
				free(info);

				uint32_t retornoMalloc = procesarMalloc(id_proceso, mensajeRecibido->tamanio);
				enviarUint(socketRespuesta, retornoMalloc);

				break;
			case FREE:
				info = malloc(strlen("[pid|%s]-> FREE sobre la dirección 999999999999999") + 1);
				sprintf(info, "[pid|%s]-> FREE sobre la dirección %u", id_proceso, mensajeRecibido->posicionMemoria);
				loggearInfo(info);
				free(info);

				int32_t retornoFree = procesarFree(id_proceso, mensajeRecibido->posicionMemoria);
				enviarInt(socketRespuesta, retornoFree);

				break;
			case GET:
				info = malloc(strlen("[pid|%s]-> GET sobre la dirección 999999999999999 de 999999999999999 bytes") + 1);
				sprintf(info, "[pid|%s]-> GET sobre la dirección %u de %d bytes", id_proceso, mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio);
				loggearInfo(info);
				free(info);

				void* retornoGet = procesarGet(id_proceso, mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio);
				enviarVoid(socketRespuesta, retornoGet, mensajeRecibido->tamanio);
				if(retornoGet)
					free(retornoGet);

				break;
			case CPY:
				info = malloc(strlen("[pid|%s]-> CPY sobre la dirección 999999999999999 de 999999999999999 bytes") + 1);
				sprintf(info, "[pid|%s]-> CPY sobre la dirección %u de %d bytes", id_proceso, mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio);
		 		loggearInfo(info);
				free(info);

				int retornoCpy = procesarCpy(id_proceso, mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio, mensajeRecibido->contenido);
				enviarInt(socketRespuesta, retornoCpy);
				free(mensajeRecibido->contenido);

				break;
			case MAP:
				info = malloc(strlen("[pid|%s]-> MAP al archivo N con el flag XXXXXXX") + 1 + strlen((char*)mensajeRecibido->contenido) + 1);
				char aux[35];
				if(mensajeRecibido->flag == MUSE_MAP_SHARED)
					strcpy(aux,"SHARED");
				else
					strcpy(aux,"PRIVATE");

				sprintf(info, "[pid|%s]-> MAP al archivo %s con el flag %s", id_proceso, (char*)mensajeRecibido->contenido, aux);
				loggearInfo(info);
				free(info);

				uint32_t retornoMap = procesarMap(id_proceso, (char*)mensajeRecibido->contenido, mensajeRecibido->tamanio, mensajeRecibido->flag);
				enviarUint(socketRespuesta, retornoMap);
				free(mensajeRecibido->contenido);

				break;
			case SYNC:
				info = malloc(strlen("[pid|%s]-> SYNC sobre la dirección 999999999999999") + 1);
				sprintf(info, "[pid|%s]-> SYNC sobre la dirección %u", id_proceso, mensajeRecibido->posicionMemoria);
				loggearInfo(info);
				free(info);

				int retornoSync = procesarSync(id_proceso, mensajeRecibido->posicionMemoria, mensajeRecibido->tamanio);
				enviarInt(socketRespuesta, retornoSync);

				break;
			case UNMAP:
				info = malloc(strlen("[pid|%s]-> UNMAP sobre la dirección 999999999999999") + 1);
				sprintf(info, "[pid|%s]-> UNMAP sobre la dirección %u", id_proceso, mensajeRecibido->posicionMemoria);
				loggearInfo(info);
				free(info);

				int retornoUnmap = procesarUnmap(id_proceso, mensajeRecibido->posicionMemoria);
				enviarInt(socketRespuesta, retornoUnmap);

				break;
			case CLOSE:
				info = malloc(strlen("[pid|%s]-> CLOSE") + 1);
				sprintf(info, "[pid|%s]-> CLOSE", id_proceso);
				loggearInfo(info);
				free(info);

				int retornoClose = procesarClose(id_proceso);
				 //funcion que debe liberar la memoria reservada tanto principal como swap y debe eliminar la entrada del diccionario
				enviarInt(socketRespuesta, retornoClose);

				break;
			default:

				break;
		}

		free(mensajeRecibido);
		//pthread_mutex_unlock(&mutex_lock_operaciones);

	}

	close(socketRespuesta);

}

void liberarTodaLaMemoria() {

	pthread_mutex_lock(&mutex_diccionario);
	void liberar(char * id, t_list * bla) {
		procesarClose(id);
	}

	dictionary_iterator(diccionarioProcesos,(void*)liberar);
	pthread_mutex_unlock(&mutex_diccionario);

	dictionary_destroy(diccionarioProcesos);
	bitarray_destroy(marcosMemoriaSwap);
	bitarray_destroy(marcosMemoriaPrincipal);

}

void salirFuncion(int pid) {

	liberarTodaLaMemoria();
	destruirLog();
	free(memoria);
	exit(1);

}
