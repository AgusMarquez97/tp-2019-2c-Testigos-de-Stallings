#ifndef MUSEOPERACIONES_H_
#define MUSEOPERACIONES_H_

#include "MuseHeapMetadata.h"

int procesarHandshake(char* idProceso) {

	char msj[120];

	if(existeEnElDiccionario(idProceso)) {
		sprintf(msj, "Fallo al realizar el handshake, init duplicado del proceso %s", idProceso);
		loggearError(msj);
		return -1;
	}

	pthread_mutex_lock(&mutex_diccionario);
	dictionary_put(diccionarioProcesos, idProceso, NULL);
	pthread_mutex_unlock(&mutex_diccionario);


	sprintf(msj, "Handshake exitoso. Proceso %s agregado al diccionario de procesos correctamente", idProceso);
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
		if(posicionRetorno == 0)
			sprintf(msj,"Error en el malloc del proceso %s: memoria llena",idProceso);
		else
			sprintf(msj,"Malloc para el proceso %s retorna la posicion %u",idProceso,posicionRetorno);

		loggearInfo(msj);
	} else	{
		sprintf(msj, "El proceso %s no realizo el init correspondiente", idProceso);
		loggearWarning(msj);
	}

	return posicionRetorno;

}

int32_t procesarFree(char* idProceso, uint32_t posicionSegmento) {

	char msj[150];

	if(poseeSegmentos(idProceso)) {
		return analizarFree(idProceso,posicionSegmento);
	}

	sprintf(msj, "El Proceso %s no ha realizado el init correspondiente", idProceso);
	loggearWarning(msj);

	return -1;

}

void* procesarGet(char* idProceso, uint32_t posicionSegmento, int32_t tamanio) {

	char msj[150];

	if(poseeSegmentos(idProceso))
	{
		return analizarGet(idProceso, posicionSegmento,tamanio);
	}
	sprintf(msj, "El Proceso %s no ah realizado el init correspondiente", idProceso);
	loggearWarning(msj);

	return NULL;

}

int procesarCpy(char* idProceso, uint32_t posicionSegmento, int32_t tamanio, void* contenido) {

	char msj[150];

	if(poseeSegmentos(idProceso))
		{
		return analizarCpy(idProceso, posicionSegmento, tamanio, contenido);
		}
		sprintf(msj, "El Proceso %s no ah realizado el init correspondiente", idProceso);
		loggearWarning(msj);
		return -1;
}

uint32_t procesarMap(char* idProceso, char* path, int32_t tamanio, int32_t flag) {

	char msj[120];
	if(existeEnElDiccionario(idProceso))
	{
	return analizarMap(idProceso, path, tamanio, flag);
	}
	sprintf(msj, "El Proceso %s no ah realizado el init correspondiente", idProceso);
	loggearWarning(msj);

	return 0;

}

int procesarSync(char* idProceso, uint32_t posicionSegmento, int32_t tamanio) {
	char msj[120];

	if(existeEnElDiccionario(idProceso))
	{
	return analizarSync(idProceso, posicionSegmento, tamanio);
	}

	sprintf(msj, "El Proceso %s no ah realizado el init correspondiente", idProceso); // ver de pasar la validacion al lado del cliente
	loggearWarning(msj);

	return -1;

}


int procesarUnmap(char* idProceso, uint32_t posicionSegmento) {

	char msj[120];

	if(existeEnElDiccionario(idProceso))
	{
	return analizarUnmap(idProceso, posicionSegmento);
	}

	sprintf(msj, "El Proceso %s no ah realizado el init correspondiente", idProceso); // ver de pasar la validacion al lado del cliente
	loggearWarning(msj);
	return 0;
}

int procesarClose(char* idProceso) {

 	char msj[120];
	t_list* segmentos;
	pthread_mutex_lock(&mutex_diccionario);
	segmentos = dictionary_get(diccionarioProcesos, idProceso);
	if(segmentos != NULL) {
		if(!list_is_empty(segmentos)) {
			void liberarSegmento(t_segmento* unSegmento) {
				if(unSegmento)
				{
					if(unSegmento->esCompartido)
					{
						if(unSegmento->archivo)
						{
							if(unSegmento->tiene_flag_shared)
								reducirArchivoCompartido(idProceso, unSegmento);
							else
								liberarConUnmap(idProceso, unSegmento);
						free(unSegmento->archivo);
						unSegmento->archivo=NULL; // por las dudas
						}
					}else{
							if(unSegmento->paginas)
							{
								list_destroy_and_destroy_elements(unSegmento->paginas, (void*)liberarPagina);
							}
						}
					free(unSegmento);
				}
			}

			list_destroy_and_destroy_elements(segmentos, (void*)liberarSegmento);
		}
	}

	dictionary_remove(diccionarioProcesos, idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	sprintf(msj, "Proceso %s eliminado del diccionario de procesos", idProceso);
	loggearInfo(msj);

	return 0;

}


#endif /* MUSEOPERACIONES_H_ */
