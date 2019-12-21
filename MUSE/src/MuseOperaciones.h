#ifndef MUSEOPERACIONES_H_
#define MUSEOPERACIONES_H_

#include "MuseHeapMetadata.h"

int procesarHandshake(char* idProceso) {

	char msj[200];

	if(existeEnElDiccionario(idProceso)) {
		sprintf(msj, "[pid|%s]-> Proceso duplicado", idProceso);
		loggearError(msj);
		return -1;
	}

	pthread_mutex_lock(&mutex_diccionario);
	dictionary_put(diccionarioProcesos, idProceso, NULL);
	pthread_mutex_unlock(&mutex_diccionario);


	sprintf(msj, "[pid|%s]-> Agregado al diccionario de procesos", idProceso);
	loggearInfo(msj);

	return 0;

}

uint32_t procesarMalloc(char* idProceso, int32_t tamanio) {

	char msj[200];
	int cantidadFrames;
	uint32_t posicionRetorno = 0;

	if(existeEnElDiccionario(idProceso)) {
		cantidadFrames = obtenerCantidadMarcos(tamPagina, tamanio + tam_heap_metadata);
		posicionRetorno = analizarSegmento(idProceso, tamanio, cantidadFrames,false);

		if(posicionRetorno == 0) {
			sprintf(msj,"[pid|%s]-> Memoria llena", idProceso);
			loggearError(msj);
		} else {
			sprintf(msj,"[pid|%s]-> Malloc en la posición %u", idProceso, posicionRetorno);
			loggearInfo(msj);
		}
	} else	{
		sprintf(msj, "[pid|%s]-> No realizo el init correspondiente", idProceso);
		loggearWarning(msj);
	}

	return posicionRetorno;

}

int32_t procesarFree(char* idProceso, uint32_t posicionSegmento) {

	char msj[200];

	if(poseeSegmentos(idProceso)) {
		return analizarFree(idProceso,posicionSegmento);
	}

	sprintf(msj, "[pid|%s]-> No realizo el init correspondiente", idProceso);
	loggearWarning(msj);

	return -1;

}

void* procesarGet(char* idProceso, uint32_t posicionSegmento, int32_t tamanio) {

	char msj[200];

	if(poseeSegmentos(idProceso)) {
		return analizarGet(idProceso, posicionSegmento,tamanio);
	}

	sprintf(msj, "[pid|%s]-> No realizo el init correspondiente", idProceso);
	loggearWarning(msj);

	return NULL;

}

int procesarCpy(char* idProceso, uint32_t posicionSegmento, int32_t tamanio, void* contenido) {

	char msj[200];

	if(poseeSegmentos(idProceso)) {
		return analizarCpy(idProceso, posicionSegmento, tamanio, contenido);
	}

	sprintf(msj, "[pid|%s]-> No realizo el init correspondiente", idProceso);
	loggearWarning(msj);

	return -1;

}

uint32_t procesarMap(char* idProceso, char* path, int32_t tamanio, int32_t flag) {

	char msj[200];

	if(existeEnElDiccionario(idProceso)) {
		return analizarMap(idProceso, path, tamanio, flag);
	}

	sprintf(msj, "[pid|%s]-> No realizo el init correspondiente", idProceso);
	loggearWarning(msj);

	return 0;

}

int procesarSync(char* idProceso, uint32_t posicionSegmento, int32_t tamanio) {

	char msj[200];

	if(existeEnElDiccionario(idProceso)) {
		return analizarSync(idProceso, posicionSegmento, tamanio);
	}

	sprintf(msj, "[pid|%s]-> No realizo el init correspondiente", idProceso);
	loggearWarning(msj);

	return -1;

}


int procesarUnmap(char* idProceso, uint32_t posicionSegmento) {

	char msj[200];

	if(existeEnElDiccionario(idProceso)) {
		return analizarUnmap(idProceso, posicionSegmento);
	}

	sprintf(msj, "[pid|%s]-> No realizo el init correspondiente", idProceso);
	loggearWarning(msj);

	return 0;

}

int procesarClose(char* idProceso) {

 	char msj[200];
	t_list* segmentos;

	pthread_mutex_lock(&mutex_diccionario);
	segmentos = dictionary_get(diccionarioProcesos, idProceso);
	if(segmentos != NULL) {
		if(!list_is_empty(segmentos)) {
			pthread_mutex_lock(&mutex_segmento);
			void liberarSegmento(t_segmento* unSegmento) {
				if(unSegmento) {
					if(unSegmento->esCompartido) {
						if(unSegmento->archivo) {
							if(unSegmento->tiene_flag_shared)
								reducirArchivoCompartido(idProceso, unSegmento);
							else
								liberarConUnmap(idProceso, unSegmento);

							free(unSegmento->archivo);
							unSegmento->archivo=NULL; // por las dudas
						}
					} else {
						if(unSegmento->paginas) {
							list_destroy_and_destroy_elements(unSegmento->paginas, (void*)liberarPagina);
						}
					}
					free(unSegmento);
				}
			}
			pthread_mutex_unlock(&mutex_segmento);
			list_destroy_and_destroy_elements(segmentos, (void*)liberarSegmento);
		}
	}

	dictionary_remove(diccionarioProcesos, idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	sprintf(msj, "[pid|%s]-> Eliminado del diccionario de procesos", idProceso);
	loggearInfo(msj);

	return 0;

}


#endif /* MUSEOPERACIONES_H_ */
