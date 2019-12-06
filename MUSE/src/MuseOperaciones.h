#ifndef MUSEOPERACIONES_H_
#define MUSEOPERACIONES_H_

#include "MuseHeapMetadata.h"

bool existeEnElDiccionario(char* idProceso) {

	if(diccionarioProcesos) {
		if(!dictionary_is_empty(diccionarioProcesos)) {
			if(dictionary_has_key(diccionarioProcesos, idProceso)) {
				return true;
			}
		}
	}

	return false;
}

int procesarHandshake(char* idProceso) {

	char msj[120];

	if(existeEnElDiccionario(idProceso)) {
		sprintf(msj, "Fallo al realizar el handshake, init duplicado del proceso %s", idProceso);
		loggearError(msj);
		return -1;
	}

	dictionary_put(diccionarioProcesos, idProceso, NULL);
	sprintf(msj, "Handshake exitoso. Proceso %s agregado al diccionario de procesos correctamente", idProceso);
	loggearInfo(msj);

	return 0;

}

uint32_t procesarMalloc(char* idProceso, int32_t tamanio) {

	char msj[120];
	int cantidadFrames;

	if(existeEnElDiccionario(idProceso)) {
		cantidadFrames = obtenerCantidadMarcos(tamPagina, tamanio + tam_heap_metadata);
		return analizarSegmento(idProceso, tamanio, cantidadFrames, false);
	}

	sprintf(msj, "El proceso %s no realizo el init correspondiente", idProceso);
	loggearWarning(msj);

	return 0;

}

void defragmentarSegmento(t_segmento* segmento) {

	t_list* listaPaginas = segmento->paginas;
	int cantPaginas = list_size(listaPaginas);
	int tamMaximo = tamPagina * cantPaginas;
	int bytesLeidos = 0;
	t_heap_metadata* heapMetadata = malloc(tam_heap_metadata);

	int offset = 0;
	int bytesLeidosPagina = 0;
	int contador = 0;
	int primerHeapMetadataLibre = 0;
	int fragmentacion = 0;

	while(tamMaximo - bytesLeidos > tam_heap_metadata) {
		leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, listaPaginas, &contador);

		if(heapMetadata->estaLibre) {
			primerHeapMetadataLibre = offset - tam_heap_metadata;
			fragmentacion += bytesLeidos;
			leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, listaPaginas, &contador);

			while(heapMetadata->estaLibre) {
				fragmentacion += bytesLeidos + tam_heap_metadata;
				leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, listaPaginas, &contador);
			}

			heapMetadata->estaLibre = 1;
			heapMetadata->offset = fragmentacion;

			pthread_mutex_lock(&mutex_memoria);
			memcpy(memoria + primerHeapMetadataLibre, heapMetadata, tam_heap_metadata);
			pthread_mutex_unlock(&mutex_memoria);

			char msj[200];
			sprintf(msj,"Se agruparon %d bytes libres",fragmentacion);
			loggearInfo(msj);

			fragmentacion = 0;
		} else {
			leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, listaPaginas, &contador);
		}
	}

	free(heapMetadata);

}

int32_t procesarFree(char* idProceso, uint32_t posicionSegmento) {

	char msj[200];
	int retorno = -1;

	if(poseeSegmentos(idProceso)) {

	int bytesLiberados = 0;
	t_list * segmentos;
	t_segmento * unSegmento;
	t_list * paginas;

	pthread_mutex_lock(&mutex_diccionario);
	segmentos = dictionary_get(diccionarioProcesos,idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	unSegmento = obtenerSegmento(segmentos,posicionSegmento); // ver de hacer validacion por el nulo
	paginas = unSegmento->paginas;

	uint32_t posicionMemoria = obtenerDireccionMemoria(paginas,posicionSegmento);

	posicionMemoria = obtenerPosicionPreviaHeap(paginas,posicionMemoria);

	bytesLiberados = liberarUnHeapMetadata(paginas, posicionMemoria);

	if(bytesLiberados > 0)
	{
		sprintf(msj, "El Proceso %s libero %d bytes en la posicion %d", idProceso,(int)(bytesLiberados-tam_heap_metadata),posicionSegmento);
		retorno = 1;
	}

	switch(bytesLiberados)
		{
		case HM_NO_EXISTENTE:
			sprintf(msj, "El Proceso %s intento liberar un HM no existente en la posicion %d", idProceso, posicionSegmento);
			break;
		case HM_YA_LIBERADO:
			sprintf(msj, "El Proceso %s intento liberar un HM que ya estaba libre en la posicion %d", idProceso, posicionSegmento);
			break;
		}

	loggearInfo(msj);

	defragmentarSegmento(unSegmento);

	return retorno;
	}

	sprintf(msj, "El Proceso %s no ha realizado el init correspondiente", idProceso);
	loggearInfo(msj);
	return retorno;

}

void* procesarGet(char* idProceso, uint32_t posicionSegmento, int32_t tamanio) {

	char msj[200];

	if(poseeSegmentos(idProceso))
	{
		int bytesLiberados = 0;
		t_list * paginas;

		pthread_mutex_lock(&mutex_diccionario);
		paginas = dictionary_get(diccionarioProcesos,idProceso);
		pthread_mutex_unlock(&mutex_diccionario);

		uint32_t posicionMemoria = obtenerDireccionMemoria(paginas,posicionSegmento);

		posicionMemoria = obtenerPosicionPreviaHeap(paginas,posicionMemoria);

		void * buffer = malloc(tamanio);

		int bytesLeidos = leerUnHeapMetadata(paginas, posicionMemoria, &buffer, tamanio);

		if(bytesLeidos > 0)
		{
			sprintf(msj, "El Proceso %s leyo %d bytes de la posicion %d", idProceso,bytesLeidos,posicionSegmento);
		}

		switch(bytesLiberados)
		{
		case HM_NO_EXISTENTE:
			sprintf(msj, "El Proceso %s intento leer de un HM no existente en la posicion %d", idProceso, posicionSegmento);
			break;
		case HM_YA_LIBERADO:
			sprintf(msj, "El Proceso %s intento leer de un HM que estaba libre en la posicion %d", idProceso, posicionSegmento);
			break;
		case TAMANIO_SOBREPASADO:
			sprintf(msj, "El Proceso %s intento leer mas bytes (%d) de los permitidos en la posicion %d", idProceso,tamanio,posicionSegmento);
			break;
		}

		return buffer;

	}
	sprintf(msj, "El Proceso %s no ah realizado el init correspondiente", idProceso);
	loggearInfo(msj);

	return NULL;

}

int procesarCpy(char* idProceso, uint32_t posicionSegmento, int32_t tamanio, void* contenido) {

	char msj[200];
	int retorno = -1;

	if(poseeSegmentos(idProceso))
		{
			int bytesLiberados = 0;
			t_list * paginas;

			pthread_mutex_lock(&mutex_diccionario);
			paginas = dictionary_get(diccionarioProcesos,idProceso);
			pthread_mutex_unlock(&mutex_diccionario);

			uint32_t posicionMemoria = obtenerDireccionMemoria(paginas,posicionSegmento);

			posicionMemoria = obtenerPosicionPreviaHeap(paginas,posicionMemoria);

			void * buffer = malloc(tamanio);

			int bytesEscritos = escribirUnHeapMetadata(paginas, posicionMemoria, &buffer, tamanio);

			if(bytesEscritos > 0)
			{
				sprintf(msj, "El Proceso %s escribio %d bytes en la posicion %d", idProceso,bytesEscritos,posicionSegmento);
				retorno = 0;
			}

			switch(bytesLiberados)
			{
			case HM_NO_EXISTENTE:
				sprintf(msj, "El Proceso %s intento escribir en un HM no existente en la posicion %d", idProceso, posicionSegmento);
				break;
			case HM_YA_LIBERADO:
				sprintf(msj, "El Proceso %s intento escribir en un HM que estaba libre en la posicion %d", idProceso, posicionSegmento);
				break;
			case TAMANIO_SOBREPASADO:
				sprintf(msj, "El Proceso %s intento escribir mas bytes (%d) de los permitidos en la posicion %d", idProceso,tamanio,posicionSegmento);
				break;
			}

		}
	sprintf(msj, "El Proceso %s no ah realizado el init correspondiente", idProceso);
	loggearInfo(msj);

	return retorno;

}

uint32_t procesarMap(char* idProceso, void* contenido, int32_t tamanio, int32_t flag) {

	char* pathArchivo = malloc(tamanio + 1);

	memcpy(pathArchivo, contenido, tamanio);
	pathArchivo[tamanio] = 0;
	loggearInfo(pathArchivo);
	free(pathArchivo);

	return 1;

}

int procesarSync(char* idProceso, uint32_t posicionMemoria, int32_t tamanio) {

	//int base = 0;
	//int offset = 0;


//	if(obtenerDireccionMemoria(idProceso, posicionMemoria, &base, &offset)) {
//		t_heap_metadata* heapMetadata = obtenerHeapMetadata(base, offset);
//
//		if(!heapMetadata->estaLibre && heapMetadata->offset >= tamanio) {
//
//			return 0;
//		}
//	}


	return -1;

}

int procesarUnmap(char* idProceso, uint32_t posicionMemoria) {

	return 0;

}

int procesarClose(char* idProceso) {

 	char msj[120];
	t_list* segmentos;

	segmentos = dictionary_get(diccionarioProcesos, idProceso);

	if(segmentos != NULL) {
		if(!list_is_empty(segmentos)) {
			void liberarSegmento(t_segmento* unSegmento) {

				void liberarPaginas(t_pagina* unaPagina) {
					liberarMarcoBitarray(unaPagina->nroMarco);
					free(unaPagina);
				}

				list_iterate(unSegmento->paginas, (void*)liberarPaginas);
				list_destroy(unSegmento->paginas);

				free(unSegmento);
			}
			list_iterate(segmentos, (void*)liberarSegmento);
			list_destroy(segmentos);
		}
	}

	dictionary_remove(diccionarioProcesos, idProceso);

	sprintf(msj, "Proceso %s eliminado del diccionario de procesos", idProceso);
	loggearInfo(msj);

	return 0;

}

/*
 * FUNCIONES MACRO DE OPERACIONES
 */

// MACRO DE MALLOC

uint32_t analizarSegmento (char* idProceso, int tamanio, int cantidadFrames, bool esCompartido) {
	//Que pasa si en el medio de esta operacion el mismo proceso mediante otro hilo me inserta otro segmento al mismo tiempo = PROBLEMAS
	char aux[140];
	uint32_t direccionRetorno = 0;
	t_list* listaSegmentos;
	int nroSegmento = 0;

	if(!poseeSegmentos(idProceso)) // => Es el primer malloc
	{
		listaSegmentos = list_create();
		crearSegmento(idProceso, tamanio, cantidadFrames, listaSegmentos, 0, esCompartido, 0);
		direccionRetorno =  tam_heap_metadata;
	}
	else
	{
		t_segmento* ultimoSegmento;
		uint32_t ultimaPosicionSegmento;

		pthread_mutex_lock(&mutex_diccionario);
		listaSegmentos = dictionary_get(diccionarioProcesos, idProceso);
		pthread_mutex_unlock(&mutex_diccionario);

		ultimoSegmento = list_get(listaSegmentos, list_size(listaSegmentos) - 1);
		ultimaPosicionSegmento = ultimoSegmento->posicionInicial + ultimoSegmento->tamanio;
		nroSegmento = ultimoSegmento->id_segmento;

		if(ultimoSegmento->esCompartido) {
			nroSegmento = ultimoSegmento->id_segmento + 1;
			ultimaPosicionSegmento = ultimoSegmento->posicionInicial + ultimoSegmento->tamanio;
			crearSegmento(idProceso, tamanio, cantidadFrames, listaSegmentos,nroSegmento, esCompartido, ultimaPosicionSegmento); // HACER
			direccionRetorno = ultimaPosicionSegmento + tam_heap_metadata;
		} else {
			direccionRetorno = completarSegmento(idProceso, ultimoSegmento, tamanio);

		}
	}

	sprintf(aux,"Malloc para el proceso %s retorna la posicion %u del segmento %d",idProceso,direccionRetorno,nroSegmento);

	if(direccionRetorno == 0)
		sprintf(aux,"Error en el malloc del proceso %s: memoria llena",idProceso);


	loggearInfo(aux);

	return direccionRetorno;

}

#endif /* MUSEOPERACIONES_H_ */
