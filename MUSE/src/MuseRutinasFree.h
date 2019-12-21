#ifndef MUSERUTINASFREE_H_
#define MUSERUTINASFREE_H_

#include "MuseMalloc.h"

int analizarFree(char* idProceso, uint32_t posicionSegmento) {

	char msj[250];
	int retorno = -1;

	int bytesLiberados = 0;
	t_list* segmentos;
	t_segmento* segmento;
	t_list* paginas;

	pthread_mutex_lock(&mutex_diccionario);
	segmentos = dictionary_get(diccionarioProcesos, idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	segmento = obtenerSegmento(segmentos, posicionSegmento); // ver de hacer validacion por el nulo

	pthread_mutex_lock(&mutex_segmento);
	if(!segmento) {
		sprintf(msj, "[pid|%s]-> Intentó liberar un segmento en la posición %d y no le pertenece", idProceso, posicionSegmento);
		loggearWarning(msj);
		return 0;
	}

	if(segmento->esCompartido) {
		sprintf(msj, "[pid|%s]-> Intentó liberar un segmento en la posición %d y es compartida", idProceso, posicionSegmento);
		loggearWarning(msj);
		return 0;
	}

	paginas = segmento->paginas;
	pthread_mutex_unlock(&mutex_segmento);

	if(posicionSegmento < tam_heap_metadata)
		return 0;

	bytesLiberados = liberarUnHeapMetadata(paginas, posicionSegmento - segmento->posicionInicial);

	if(bytesLiberados > 0) {
		sprintf(msj, "[pid|%s]-> Liberó %d bytes de la posición %d", idProceso, (int)(bytesLiberados - tam_heap_metadata), posicionSegmento);
		retorno = 1;
	} else {
		strcpy(msj, "");
		switch(bytesLiberados) {
			case HM_NO_EXISTENTE:
				sprintf(msj, "[pid|%s]-> Intentó liberar un HeapMetadata en la posición %d y éste no existe", idProceso, posicionSegmento);
			break;
		case HM_YA_LIBERADO:
				sprintf(msj, "[pid|%s]-> Intentó liberar un HeapMetadata en la posición %d y éste ya está libre", idProceso, posicionSegmento);
			break;
		}

		loggearWarning(msj);
		return -1;
	}

	loggearInfo(msj);

	int bytesAgrupados = defragmentarSegmento(segmento);

	if(bytesAgrupados > 0) {
		int segundosBytesAgrupados = defragmentarSegmento(segmento);

		if(segundosBytesAgrupados > 0)
			bytesAgrupados = segundosBytesAgrupados;

		sprintf(msj, "[pid|%s]-> Se agruparon %d bytes libres contiguos", idProceso, bytesAgrupados);
		loggearInfo(msj);
	}

	compactarSegmento(idProceso, segmento);

	return retorno;

}

int liberarUnHeapMetadata(t_list * paginas, int offsetSegmento) {

	int nroPaginaActual = obtenerNroPagina(paginas,offsetSegmento);
	int offsetPrevio = obtenerOffsetPrevio(paginas,offsetSegmento,nroPaginaActual);

	if(existeHM(paginas, offsetPrevio)) {
		t_heap_metadata * unHeap = obtenerHeapMetadata(paginas, offsetPrevio,nroPaginaActual);

		int tamanioPaginaRestante = tamPagina - offsetSegmento%tamPagina;

		if(!unHeap->estaLibre) {
			unHeap->estaLibre = true;
			int tmp = escribirUnHeapMetadata(paginas, nroPaginaActual, unHeap, &offsetPrevio, tamanioPaginaRestante);
			free(unHeap);
			return tmp;
		}

		free(unHeap);

		return HM_YA_LIBERADO;

	}

	return HM_NO_EXISTENTE;
}

int defragmentarSegmento(t_segmento* segmento) {

	pthread_mutex_lock(&mutex_segmento);
	t_list* listaPaginas = segmento->paginas;
	pthread_mutex_unlock(&mutex_segmento);

	t_pagina* paginaAuxiliar = obtenerPaginaAuxiliar(listaPaginas, 0);

	int offset = paginaAuxiliar->nroMarco * tamPagina;

	free(paginaAuxiliar);

	pthread_mutex_lock(&mutex_segmento);
	int cantPaginas = list_size(listaPaginas);
	int tamMaximo = tamPagina * cantPaginas;
	pthread_mutex_unlock(&mutex_segmento);

	int bytesLeidos = 0;
	t_heap_metadata* heapMetadata = malloc(tam_heap_metadata);

	int bytesLeidosPagina = 0;
	int contador = 0;
	int heapsLeidos = 0;
	int primerHeapMetadataLibre = 0;
	int primeraPaginaLibre = 0;
	int acumulador = 0;
	int cantidadBytesAgrupados = 0;
	int paginaUltimoHeapMetadata = 0;

	int offsetAnterior = 0;
	while(tamMaximo - bytesLeidos > tam_heap_metadata)
	{
		offsetAnterior = offset;
		paginaUltimoHeapMetadata = contador;
		leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, listaPaginas, &contador);

		if(heapMetadata->estaLibre)
		{
			heapsLeidos++;

			if(heapsLeidos == 1)
			{
				primeraPaginaLibre = paginaUltimoHeapMetadata;
				primerHeapMetadataLibre = offsetAnterior;
			}


			acumulador += heapMetadata->offset;

			if(heapsLeidos > 1)
			{
				heapMetadata->estaLibre = true;
				heapMetadata->offset = acumulador + tam_heap_metadata;

				int tamanioRestante = tamPagina - primerHeapMetadataLibre%tamPagina;

				escribirUnHeapMetadata(listaPaginas, primeraPaginaLibre, heapMetadata, &primerHeapMetadataLibre, tamanioRestante);
				cantidadBytesAgrupados = heapMetadata->offset;

				break;
			}
		}
		else
		{
			heapsLeidos = 0;
			acumulador = 0;
		}
	}
	free(heapMetadata);

	return cantidadBytesAgrupados;

}

void compactarSegmento(char* idProceso, t_segmento* segmento) {

	pthread_mutex_lock(&mutex_segmento);
	t_list* listaPaginas = segmento->paginas;
	pthread_mutex_unlock(&mutex_segmento);

	t_pagina* paginaAuxiliar = obtenerPaginaAuxiliar(listaPaginas, 0);

	int offset = paginaAuxiliar->nroMarco * tamPagina;

	free(paginaAuxiliar);

	t_heap_metadata* heapMetadata = malloc(tam_heap_metadata);
	int bytesLeidos = 0;
	int bytesLeidosPagina = 0;

	pthread_mutex_lock(&mutex_segmento);
	t_list* paginas = segmento->paginas;
	int cantPaginas = list_size(paginas);
	pthread_mutex_unlock(&mutex_segmento);

	int tamMaximo = tamPagina * cantPaginas;
	int nroPagina = 0;

	int posUltimoHeapMetadata = 0;
	int paginaUltimoHeapMetadata = 0;
	int tamanioPaginaRestante = 0;

	while(tamMaximo - bytesLeidos > tam_heap_metadata) {
		posUltimoHeapMetadata = offset;
		paginaUltimoHeapMetadata = nroPagina;
		leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, paginas, &nroPagina);
	}

	if(heapMetadata->estaLibre && nroPagina > paginaUltimoHeapMetadata) {
		tamanioPaginaRestante = tamPagina - posUltimoHeapMetadata%tamPagina;

		if(tamanioPaginaRestante > tam_heap_metadata) {
		heapMetadata->offset = tamanioPaginaRestante - tam_heap_metadata;
		escribirUnHeapMetadata(paginas, paginaUltimoHeapMetadata, heapMetadata, &posUltimoHeapMetadata, tamanioPaginaRestante);
		}

		liberarPaginas(idProceso, paginaUltimoHeapMetadata, segmento);
	}

	free(heapMetadata);

}


void liberarPaginas(char* idProceso, int nroPagina, t_segmento* segmento) {

	char msj[450];
	char aux[100];
	pthread_mutex_lock(&mutex_segmento);
	t_list* paginas = segmento->paginas;

	if((nroPagina + 1) == list_size(paginas)) {
		sprintf(msj, "[pid|%s]-> Se ha liberado la página ", idProceso);
	} else {
		sprintf(msj, "[pid|%s]-> Se han liberado las páginas [ ", idProceso);
	}

	bool buscarPagina(t_pagina* pagina) {
		return (pagina->nroPagina <= nroPagina);
	}

	t_list* lista_aux = list_filter(paginas, (void*)buscarPagina);

	void liberarPaginaLocal(t_pagina* pagina) {

		if(pagina->nroPagina > nroPagina) {

			eliminarDeAlgoritmo(pagina);

			sprintf(aux, "%d ",pagina->nroPagina);
			strcat(msj, aux);

			if(pagina->nroPaginaSwap == -1)
				liberarMarcoBitarray(pagina->nroMarco);
			else if(pagina->nroPaginaSwap >= 0)
				liberarPaginasSwap(pagina->nroPaginaSwap);
				//existe el caso donde no este en ningun lado => no se descargo el archivo todavia

			free(pagina);

		}
	}

	list_destroy_and_destroy_elements(paginas, (void*)liberarPaginaLocal);

	segmento->paginas = list_duplicate(lista_aux);
	segmento->tamanio = tamPagina*list_size(segmento->paginas);

	list_destroy(lista_aux);

	pthread_mutex_unlock(&mutex_segmento);

	strcat(msj, "]");

	loggearInfo(msj);

}

void liberarPagina(t_pagina* pagina) {

	eliminarDeAlgoritmo(pagina);

	if(pagina->nroPaginaSwap == -1)
		liberarMarcoBitarray(pagina->nroMarco);
	else if(pagina->nroPaginaSwap >= 0)
		liberarPaginasSwap(pagina->nroPaginaSwap);
		//existe el caso donde no este en ningun lado => no se descargo el archivo todavia

	free(pagina);

}

#endif /* MUSERUTINASFREE_H_ */
