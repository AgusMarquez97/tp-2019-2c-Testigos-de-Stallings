#ifndef MUSECPYGET_H_
#define MUSECPYGET_H_

#include "MuseRutinasFree.h"

void* analizarGet(char* idProceso, uint32_t posicionSegmento, int32_t tamanio) {

	char msj[200];

	t_list* paginas;
	t_list* segmentos;
	t_segmento* segmento;

	void* buffer = malloc(tamanio);

	pthread_mutex_lock(&mutex_diccionario);
	segmentos = dictionary_get(diccionarioProcesos,idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	segmento = obtenerSegmento(segmentos, posicionSegmento); // ver de hacer validacion por el nulo

	if(!segmento) {
		sprintf(msj, "[pid|%s]-> Intentó leer mas bytes (%d) de los permitidos en la posición %d", idProceso, tamanio, posicionSegmento);
		loggearWarning(msj);
		return NULL;
	}

	paginas = segmento->paginas;

	int bytesLeidos;

	if(segmento->esCompartido) {
		bytesLeidos = analizarGetMemoriaMappeada(idProceso, segmento, posicionSegmento, tamanio, &buffer);
	} else {
		bytesLeidos = leerUnHeapMetadata(paginas, posicionSegmento - segmento->posicionInicial, &buffer, tamanio);
	}

	if(bytesLeidos > 0) {
		sprintf(msj, "[pid|%s]-> Lectura sobre la posición %d de %d bytes", idProceso, posicionSegmento, bytesLeidos);
		loggearInfo(msj);
		return buffer;
	}

	strcpy(msj,"");

	switch(bytesLeidos) {
		case HM_NO_EXISTENTE:
			sprintf(msj, "[pid|%s]-> Intentó leer de un HeapMetadata que no existe en la posicion %d", idProceso, posicionSegmento);
			break;
		case HM_YA_LIBERADO:
			sprintf(msj, "[pid|%s]-> Intentó leer de un HeapMetadata que esta libre en la posición %d", idProceso, posicionSegmento);
			break;
		case TAMANIO_SOBREPASADO:
			sprintf(msj, "[pid|%s]-> Intentó leer mas bytes (%d) de los permitidos en la posición %d", idProceso, tamanio, posicionSegmento);
			break;
	}

	loggearWarning(msj);
	return NULL;

}

int leerUnHeapMetadata(t_list * paginas, int posicionRelativaSegmento, void ** buffer, int tamanio) {

	int nroPaginaActual = (int) posicionRelativaSegmento / tamPagina;
	int offsetMemoria = obtenerOffsetPosterior(paginas, posicionRelativaSegmento, nroPaginaActual); // posicion luego del heap en memoria
	int cantidadBytesRestantes = 0;

	t_heap_metadata* unHeap = recuperarHeapMetadata(paginas, posicionRelativaSegmento, &cantidadBytesRestantes);

	if(unHeap->estaLibre) {
		free(unHeap);
		return HM_YA_LIBERADO;
	}

	if(tamanio > cantidadBytesRestantes) {
		free(unHeap);
		return TAMANIO_SOBREPASADO;
	}

	leerDatosMemoria(paginas, nroPaginaActual, offsetMemoria, buffer, tamanio);
	free(unHeap);

	return tamanio; // si llego nunca deberia fallar leerDatosHeap

}


void leerDatosMemoria(t_list* paginas, int paginaActual, int posicionMemoria, void** buffer, int tamanio) {

	t_pagina* unaPagina = obtenerPaginaAuxiliar(paginas, paginaActual);
	t_pagina* paginaAux;

	int bytesRestantesPagina = 0;
	int bytesLeidos = 0;

	if(tamanio > tamPagina)
		bytesRestantesPagina = tamPagina - posicionMemoria%tamPagina;
	else
		bytesRestantesPagina = tamanio;

	while(bytesLeidos < tamanio) {

		pthread_mutex_lock(&mutex_memoria);
		paginaAux = list_get(paginas, paginaActual);

		if(!estaEnMemoria(paginas, paginaActual)) {
			rutinaReemplazoPaginasSwap(&paginaAux);
		}

		posicionMemoria = paginaAux->nroMarco * tamPagina + (posicionMemoria)%tamPagina; // sumo base mas offset
		paginaAux->uso = 1;
		memcpy(*buffer + bytesLeidos, memoria + posicionMemoria, bytesRestantesPagina);
		pthread_mutex_unlock(&mutex_memoria);

		bytesLeidos += bytesRestantesPagina;

		if(bytesLeidos == tamanio)
			break;

		free(unaPagina);
		paginaActual++;

		unaPagina = obtenerPaginaAuxiliar(paginas, paginaActual);

		posicionMemoria = unaPagina->nroMarco * tamPagina; // me paro en la primera posicion de la siguiente pagina

		if(tamanio - bytesLeidos < tamPagina)
			bytesRestantesPagina = tamanio - bytesLeidos;
		else
			bytesRestantesPagina = tamPagina;

	}

	free(unaPagina);

}


int analizarCpy(char* idProceso, uint32_t posicionSegmento, int32_t tamanio, void* contenido) {

	char msj[250];

	pthread_mutex_lock(&mutex_diccionario);
	t_list* segmentos = dictionary_get(diccionarioProcesos, idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	t_segmento* segmento = obtenerSegmento(segmentos, posicionSegmento); // ver de hacer validacion por el nulo

	if(!segmento) {
		sprintf(msj, "[pid|%s]-> Intentó escribir fuera del segmento en la direccion %d", idProceso, posicionSegmento);
		loggearWarning(msj);

		return -1;
	}

	t_list * paginas = segmento->paginas;

	int bytesEscritos;

	if(segmento->esCompartido) {
		bytesEscritos = analizarCpyMemoriaMappeada(idProceso, segmento, posicionSegmento, tamanio, &contenido);
	} else {
		bytesEscritos = escribirDatosHeapMetadata(paginas,posicionSegmento - segmento->posicionInicial, &contenido, tamanio);
	}

	if(bytesEscritos > 0) {
		sprintf(msj, "[pid|%s]-> Escritura sobre la posición %d de %d bytes", idProceso, posicionSegmento, bytesEscritos);
		loggearInfo(msj);

		return 0;
	}

	strcpy(msj,"");

	switch(bytesEscritos) {
		case HM_NO_EXISTENTE:
			sprintf(msj, "[pid|%s]-> Intentó escribir en un HeapMetadata que no existe en la posicion %d", idProceso, posicionSegmento);
			break;
		case HM_YA_LIBERADO:
			sprintf(msj, "[pid|%s]-> Intentó escribir en un HeapMetadata que está libre en la posición %d", idProceso, posicionSegmento);
			break;
		case TAMANIO_SOBREPASADO:
			sprintf(msj, "[pid|%s]-> Intentó escribir más bytes (%d) de los permitidos en la posición %d", idProceso, tamanio, posicionSegmento);
			break;
	}

	loggearWarning(msj);
	return -1;

}


int escribirDatosHeapMetadata(t_list* paginas, int posicionRelativaSegmento, void** buffer, int tamanio) {

	int nroPaginaActual = (int) posicionRelativaSegmento / tamPagina;
	int offsetMemoria = obtenerOffsetPosterior(paginas, posicionRelativaSegmento, nroPaginaActual); // posicion luego del heap en memoria
	int cantidadBytesRestantes = 0;

	t_heap_metadata* unHeap = recuperarHeapMetadata(paginas, posicionRelativaSegmento, &cantidadBytesRestantes);

	if(unHeap->estaLibre) {
		free(unHeap);
		return HM_YA_LIBERADO;
	}

	if(tamanio > cantidadBytesRestantes) {
		free(unHeap);
		return TAMANIO_SOBREPASADO;
	}

	escribirDatosHeap(paginas, nroPaginaActual, offsetMemoria, buffer, tamanio);
	free(unHeap);

	return tamanio; // si llego nunca deberia fallar leerDatosHeap

}

void escribirDatosHeap(t_list* paginas, int paginaActual, int posicionPosteriorHeap, void** buffer, int tamanio) {

	t_pagina* unaPagina = obtenerPaginaAuxiliar(paginas, paginaActual);
	int bytesEscritos = 0;
	int bytesRestantesPagina = 0;
	t_pagina* paginaAux;

	if(tamanio > tamPagina)
		bytesRestantesPagina = tamPagina - posicionPosteriorHeap%tamPagina;
	else
		bytesRestantesPagina = tamanio;

	while(bytesEscritos < tamanio) {

		pthread_mutex_lock(&mutex_memoria);
		paginaAux = list_get(paginas, paginaActual);

		if(!estaEnMemoria(paginas, paginaActual)) {
			rutinaReemplazoPaginasSwap(&paginaAux);
		}

		posicionPosteriorHeap = paginaAux->nroMarco * tamPagina + (posicionPosteriorHeap) % tamPagina; // sumo base mas offset
		paginaAux->modificada = true;
		memcpy(memoria + posicionPosteriorHeap, *buffer + bytesEscritos, bytesRestantesPagina);
		pthread_mutex_unlock(&mutex_memoria);

		bytesEscritos += bytesRestantesPagina;

		if(bytesEscritos == tamanio)
			break;

		free(unaPagina);
		paginaActual++;

		unaPagina = obtenerPaginaAuxiliar(paginas, paginaActual);

		posicionPosteriorHeap = unaPagina->nroMarco * tamPagina; // me paro en la primera posicion de la siguiente pagina

		if(tamanio - bytesEscritos < tamPagina)
			bytesRestantesPagina = tamanio - bytesEscritos;
		else
			bytesRestantesPagina = tamPagina;
	}

	free(unaPagina);

}

t_heap_metadata* recuperarHeapMetadata(t_list* listaPaginas, uint32_t cantidadBytes, int* cantidadBytesRestantes) {

	t_pagina* unaPagina = list_get(listaPaginas, 0);

	int offset = unaPagina->nroMarco * tamPagina;
	int bytesLeidos = 0;
	t_heap_metadata* heapMetadata = malloc(tam_heap_metadata);
	int contador = 0;
	int bytesLeidosPagina = 0;

	while(cantidadBytes > bytesLeidos) {
		leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, listaPaginas, &contador);
	}

	*cantidadBytesRestantes = (bytesLeidos - cantidadBytes); // mi tamaño maximo restante por leer desde donde estoy parado

	return heapMetadata;

}

int analizarGetMemoriaMappeada(char* idProceso, t_segmento* unSegmento, uint32_t posicionRelativaSegmento, int32_t tamanio, void** buffer) {

	int tamanioMaximo = (unSegmento->tamanio + unSegmento->posicionInicial) - posicionRelativaSegmento; // como lo sabria??

	if(tamanio > tamanioMaximo)
		return TAMANIO_SOBREPASADO;

	posicionRelativaSegmento -= unSegmento->posicionInicial;

	int nroPaginaActual = (int) posicionRelativaSegmento / tamPagina;

	t_list* listaPaginas = unSegmento->paginas;

	leerDatosMemoria(listaPaginas, nroPaginaActual, posicionRelativaSegmento, buffer, tamanio);

	return tamanio;

}

int analizarCpyMemoriaMappeada(char* idProceso, t_segmento* unSegmento, uint32_t posicionRelativaSegmento, int32_t tamanio, void** contenidoACopiar) {

	int tamanioMaximo = (unSegmento->tamanio + unSegmento->posicionInicial) - posicionRelativaSegmento; // como lo sabria??

	if(tamanio > tamanioMaximo)
		return TAMANIO_SOBREPASADO;

	posicionRelativaSegmento -= unSegmento->posicionInicial;

	int nroPaginaActual = (int) posicionRelativaSegmento / tamPagina;

	t_list* listaPaginas = unSegmento->paginas;

	t_pagina* unaPagina = list_get(listaPaginas, nroPaginaActual);

	escribirDatosHeap(listaPaginas, nroPaginaActual, unaPagina->nroMarco*tamPagina + posicionRelativaSegmento % tamPagina, contenidoACopiar, tamanio);

	return tamanio;

}

#endif /* MUSECPYGET_H_ */
