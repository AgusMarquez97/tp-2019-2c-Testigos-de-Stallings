/*
 * MuseRutinasFree.h
 *
 *  Created on: Dec 17, 2019
 *      Author: agus
 */

#ifndef MUSERUTINASFREE_H_
#define MUSERUTINASFREE_H_

#include "MuseMalloc.h"

int defragmentarSegmento(t_segmento* segmento)
{
	t_list* listaPaginas = segmento->paginas;

	t_pagina* paginaAuxiliar = obtenerPaginaAuxiliar(listaPaginas, 0);

	int offset = paginaAuxiliar->nroMarco * tamPagina;

	free(paginaAuxiliar);

	int cantPaginas = list_size(listaPaginas);
	int tamMaximo = tamPagina * cantPaginas;
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

	t_list* listaPaginas = segmento->paginas;

	t_pagina* paginaAuxiliar = obtenerPaginaAuxiliar(listaPaginas, 0);

	int offset = paginaAuxiliar->nroMarco * tamPagina;

	free(paginaAuxiliar);

	t_heap_metadata* heapMetadata = malloc(tam_heap_metadata);
	int bytesLeidos = 0;
	int bytesLeidosPagina = 0;
	t_list* paginas = segmento->paginas;
	int cantPaginas = list_size(paginas);
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

		if(tamanioPaginaRestante>tam_heap_metadata)
		{
		heapMetadata->offset = tamanioPaginaRestante - tam_heap_metadata; // ver de restar uno
		escribirUnHeapMetadata(paginas, paginaUltimoHeapMetadata, heapMetadata, &posUltimoHeapMetadata, tamanioPaginaRestante);
		}

		liberarPaginas(idProceso, paginaUltimoHeapMetadata, segmento);
	}

	free(heapMetadata);

}


void liberarPaginas(char* idProceso, int nroPagina, t_segmento* segmento) {

	char msj[450];
	char aux[100];

	t_list * paginas = segmento->paginas;

	if((nroPagina + 1) == list_size(paginas)) {
		sprintf(msj, "Para el proceso %s, se ha liberado la página ", idProceso);
	} else {
		sprintf(msj, "Para el proceso %s, se han liberado las páginas [ ", idProceso);
	}

	bool buscarPagina(t_pagina* pagina) {
		return (pagina->nroPagina <= nroPagina);
	}

	t_list* lista_aux = list_filter(paginas, (void*)buscarPagina);

	void liberarPagina(t_pagina* pagina) {
		if(pagina->nroPagina > nroPagina) {
			sprintf(aux, "%d ",pagina->nroPagina);
			strcat(msj, aux);
			liberarMarcoBitarray(pagina->nroMarco);
			free(pagina);
		}
	}

	list_destroy_and_destroy_elements(paginas, (void*)liberarPagina);

	segmento->paginas = list_duplicate(lista_aux); //OJO! POSIBLE ML
	segmento->tamanio = tamPagina*list_size(segmento->paginas);

	list_destroy(lista_aux);

	strcat(msj, "]");

	loggearInfo(msj);

}

#endif /* MUSERUTINASFREE_H_ */
