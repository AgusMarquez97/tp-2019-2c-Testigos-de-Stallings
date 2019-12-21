/*
 * MuseHeapMetadata.h
 *
 *  Created on: Dec 5, 2019
 *      Author: agus
 */

#ifndef MUSEHEAPMETADATA_H_
#define MUSEHEAPMETADATA_H_

#include "MuseMemoriaSwap.h"

/*
 * LECTURA HM
 */


void leerHeapMetadata(t_heap_metadata** heapMetadata, int* bytesLeidos, int* bytesLeidosPagina, int* offset, t_list * paginas, int* nroPagina)
{

	int bytesIniciales = *bytesLeidosPagina;
	int incremento = 0; // lo que me paso cuando paso de pagina
	int paginasSalteadas = 0;
	t_pagina* paginaDummy = malloc(sizeof(*paginaDummy));
	int sobrantePaginaInicial = tamPagina - (*bytesLeidosPagina);
	int nroMarco = 0;
	t_pagina * paginaAux = NULL;
	t_pagina * ex;

	if(sobrantePaginaInicial == 0)
	{
		(*nroPagina)++;
		sobrantePaginaInicial = tamPagina;
		ex = obtenerPaginaAuxiliar(paginas, (*nroPagina));
		*offset = ex->nroMarco * tamPagina;
	}

	if(sobrantePaginaInicial<tam_heap_metadata) { // esta partido el proximo heap
		leerHeapPartido(heapMetadata, offset, sobrantePaginaInicial, nroPagina, paginas, &paginaDummy);
		incremento = (tam_heap_metadata - sobrantePaginaInicial) + (*heapMetadata)->offset;
		*bytesLeidosPagina = incremento;
		bytesIniciales = 0;
	} else {

		pthread_mutex_lock(&mutex_memoria);
		paginaAux = list_get(paginas,*nroPagina);
		if(!estaEnMemoria(paginas,*nroPagina))
		{
			rutinaReemplazoPaginasSwap(&paginaAux);
		}
		*offset = paginaAux->nroMarco*tamPagina + (*offset)%tamPagina; // sumo base mas offse
		memcpy(*heapMetadata, memoria + (*offset), tam_heap_metadata);
		pthread_mutex_unlock(&mutex_memoria);

		*bytesLeidosPagina += tam_heap_metadata + (*heapMetadata)->offset;
		incremento = *bytesLeidosPagina - bytesIniciales; // si paso de pagina => se va a recalcular
	}

	*bytesLeidos += tam_heap_metadata + (*heapMetadata)->offset;
	free(paginaDummy);

	if(*bytesLeidosPagina > tamPagina) { // se paso de la pagina en la que estaba
		paginasSalteadas = obtenerCantidadMarcos(tamPagina,*bytesLeidosPagina)-1;
		incremento = *bytesLeidosPagina - tamPagina*(paginasSalteadas);
		(*nroPagina) += paginasSalteadas;

		if(paginasSalteadas == list_size(paginas)) {
			nroMarco = list_size(paginas);
		} else {
			paginaDummy = obtenerPaginaAuxiliar(paginas, (*nroPagina));
			nroMarco = paginaDummy->nroMarco;
			free(paginaDummy);
		}
		*offset = nroMarco * tamPagina + incremento;
		*bytesLeidosPagina = incremento;
	} else {
		*offset += incremento;
	}

}
//prueba
void leerHeapPartido(t_heap_metadata** heapMetadata, int* offset, int sobrante, int* nroPagina, t_list* paginas, t_pagina** paginaDummy)
{

	void* buffer = malloc(tam_heap_metadata);
	t_pagina * paginaAux = NULL;

	pthread_mutex_lock(&mutex_memoria);
	paginaAux = list_get(paginas,*nroPagina);
	if(!estaEnMemoria(paginas,*nroPagina))
	{
		rutinaReemplazoPaginasSwap(&paginaAux);
	}
	*offset = paginaAux->nroMarco*tamPagina + (*offset)%tamPagina; // sumo base mas offse
	memcpy(buffer, memoria + (*offset), sobrante);
	pthread_mutex_unlock(&mutex_memoria);

	(*nroPagina)++;

	free(*paginaDummy);

	(*paginaDummy) = obtenerPaginaAuxiliar(paginas, (*nroPagina));
	*offset = (*paginaDummy)->nroMarco * tamPagina;

	pthread_mutex_lock(&mutex_memoria);
	paginaAux = list_get(paginas,*nroPagina);
	if(!estaEnMemoria(paginas,*nroPagina))
	{
		rutinaReemplazoPaginasSwap(&paginaAux);
	}
	*offset = paginaAux->nroMarco*tamPagina + (*offset)%tamPagina; // sumo base mas offse
	memcpy(buffer + sobrante, memoria + (*offset), tam_heap_metadata - sobrante);
	pthread_mutex_unlock(&mutex_memoria);

	memcpy(*heapMetadata, buffer, tam_heap_metadata);

	free(buffer);

}

/*
 * ESCRITURA HM
 */


//Escribe un solo heap
int escribirUnHeapMetadata(t_list * listaPaginas,int paginaActual,t_heap_metadata * unHeapMetadata, int * offset, int tamanioPaginaRestante)
{
	t_pagina * paginaAux = NULL;
	if(tamanioPaginaRestante >= tam_heap_metadata)
	{
		pthread_mutex_lock(&mutex_memoria);
		paginaAux = list_get(listaPaginas,paginaActual);
		if(!estaEnMemoria(listaPaginas,paginaActual))
		{
			rutinaReemplazoPaginasSwap(&paginaAux);
		}
		*offset = paginaAux->nroMarco*tamPagina + (*offset)%tamPagina; // sumo base mas offset
		memcpy(memoria + *offset,unHeapMetadata,tam_heap_metadata);
		pthread_mutex_unlock(&mutex_memoria);
		*offset += tam_heap_metadata;
	}
	else
	{
		void * buffer = malloc(tam_heap_metadata);
		int offsetBuffer = 0;

		memcpy(buffer,unHeapMetadata,tam_heap_metadata);

		pthread_mutex_lock(&mutex_memoria);
		paginaAux = list_get(listaPaginas,paginaActual);
		if(!estaEnMemoria(listaPaginas,paginaActual))
		{
			rutinaReemplazoPaginasSwap(&paginaAux);
		}
		*offset = paginaAux->nroMarco*tamPagina + (*offset)%tamPagina; // sumo base mas offset
		memcpy(memoria + *offset,buffer,tamanioPaginaRestante);
		pthread_mutex_unlock(&mutex_memoria);

		offsetBuffer+=tamanioPaginaRestante;
		paginaActual++;

		t_pagina * pagina = obtenerPaginaAuxiliar(listaPaginas,paginaActual);

		*offset = pagina->nroMarco*tamPagina; //Obtengo la pagina siguiente

		pthread_mutex_lock(&mutex_memoria);
		paginaAux = list_get(listaPaginas,paginaActual);
		if(!estaEnMemoria(listaPaginas,paginaActual))
		{
			rutinaReemplazoPaginasSwap(&paginaAux);
		}
		*offset = paginaAux->nroMarco*tamPagina + (*offset)%tamPagina; // sumo base mas offset
		memcpy(memoria + *offset,buffer + offsetBuffer,tam_heap_metadata - offsetBuffer);
		pthread_mutex_unlock(&mutex_memoria);

		*offset += (tam_heap_metadata - offsetBuffer);

		free(buffer);
		free(pagina);
	}

	return tam_heap_metadata + unHeapMetadata->offset;
}


// El offset tiene que estar justo en el valor anterior al hm
int escribirHeapMetadata(t_list * listaPaginas,int paginaActual, int offset, int tamanio, int offsetMaximo)
{
		t_pagina* paginaAuxiliar = obtenerPaginaAuxiliar(listaPaginas, paginaActual);

		int marcoSiguiente = paginaAuxiliar->nroMarco + 1;

		free(paginaAuxiliar);

		t_heap_metadata * unHeapMetadata;

		int paginasPedidas = cantidadPaginasPedidas(tamanio+tam_heap_metadata); // Por que no considera el tam_heap??
		int bytesSobrantesUltimaPagina;
		int tamanioPaginaRestante = (tamPagina)*(marcoSiguiente) - offset;
		int bytesEscritos = 0;

		int posicionActual = offset; // ??

		unHeapMetadata = malloc(tam_heap_metadata);
		unHeapMetadata->estaLibre = false;
		unHeapMetadata->offset = tamanio;

		if(offsetMaximo != 0)
		{
			paginasPedidas = cantidadPaginasPedidas(offsetMaximo); // VALIDAR
			if(tamanioPaginaRestante > offsetMaximo)
				tamanioPaginaRestante = offsetMaximo;
		}

		escribirUnHeapMetadata(listaPaginas, paginaActual, unHeapMetadata, &offset, tamanioPaginaRestante);

		bytesEscritos = tam_heap_metadata + tamanio;

		if(bytesEscritos > tamanioPaginaRestante) // => pasaste de pagina!
		{
			if(paginasPedidas == 0)
			paginasPedidas++;

			bytesSobrantesUltimaPagina = tamPagina*(paginasPedidas) - (bytesEscritos - tamanioPaginaRestante);

			if(bytesSobrantesUltimaPagina<0)
			{
				bytesSobrantesUltimaPagina += tamPagina;
				paginasPedidas++;
			}
		}
		else
		{
			bytesSobrantesUltimaPagina = tamanioPaginaRestante - bytesEscritos; // bytes que me sobraron en la pagina actual
		}

		if(bytesSobrantesUltimaPagina >= tam_heap_metadata || offsetMaximo!=0)
		{
			t_pagina * pagina = obtenerPaginaAuxiliar(listaPaginas,paginaActual + paginasPedidas); // La idea es ir siempre a la ultima pagina donde este el heap

			offset = pagina->nroMarco*tamPagina + (tamPagina - bytesSobrantesUltimaPagina);

			unHeapMetadata->estaLibre = true;

			if(offsetMaximo!=0)
			{
				if(offsetMaximo<tamPagina)
				offset = pagina->nroMarco*tamPagina + (offsetMaximo - bytesSobrantesUltimaPagina) + posicionActual%tamPagina;
				unHeapMetadata->offset = (offsetMaximo - bytesEscritos) - tam_heap_metadata; // PUEDE ESTAR PARTIDO => ANDA BUSCANDO LA 9mm
			}
			else
				unHeapMetadata->offset = bytesSobrantesUltimaPagina - tam_heap_metadata; // PUEDE ESTAR PARTIDO => ANDA BUSCANDO LA 9mm

			escribirUnHeapMetadata(listaPaginas,pagina->nroPagina,unHeapMetadata, &offset, bytesSobrantesUltimaPagina);
			bytesEscritos += bytesSobrantesUltimaPagina;

			free(pagina);
		}

		free(unHeapMetadata);

		return bytesEscritos;
}

/*
 * Obtener HM
 */

t_heap_metadata * obtenerHeapMetadata(t_list * listaPaginas, int offsetPrevioHM, int nroPagina)
{
	t_heap_metadata * unHeapMetadata = malloc(tam_heap_metadata);

	t_pagina * pagina = obtenerPaginaAuxiliar(listaPaginas, nroPagina);
	t_pagina * paginaAux = NULL;
	int tamanioPaginaRestante = (tamPagina)*(pagina->nroMarco+1) - offsetPrevioHM;

	if(tamanioPaginaRestante >= tam_heap_metadata)
	{

		pthread_mutex_lock(&mutex_memoria);
		paginaAux = list_get(listaPaginas,nroPagina);
		if(!estaEnMemoria(listaPaginas,nroPagina))
		{
			rutinaReemplazoPaginasSwap(&paginaAux);
		}
		offsetPrevioHM = paginaAux->nroMarco*tamPagina + (offsetPrevioHM)%tamPagina; // sumo base mas offset
		memcpy(unHeapMetadata,memoria + offsetPrevioHM,tam_heap_metadata);
		pthread_mutex_unlock(&mutex_memoria);

	}
	else
	{
		void * buffer = malloc(tam_heap_metadata);
		int offsetBuffer = 0;

		pthread_mutex_lock(&mutex_memoria);
		paginaAux = list_get(listaPaginas,nroPagina);
		if(!estaEnMemoria(listaPaginas,nroPagina))
		{
			rutinaReemplazoPaginasSwap(&paginaAux);
		}
		offsetPrevioHM = paginaAux->nroMarco*tamPagina + (offsetPrevioHM)%tamPagina; // sumo base mas offset
		memcpy(buffer,memoria + offsetPrevioHM,tamanioPaginaRestante);
		pthread_mutex_unlock(&mutex_memoria);

		offsetBuffer+=tamanioPaginaRestante;
		free(pagina);

		nroPagina++;

		pagina = obtenerPaginaAuxiliar(listaPaginas,nroPagina);
		offsetPrevioHM = pagina->nroMarco*tamPagina; //Obtengo la pagina siguiente

		pthread_mutex_lock(&mutex_memoria);
		paginaAux = list_get(listaPaginas,nroPagina);
		if(!estaEnMemoria(listaPaginas,nroPagina))
		{
			rutinaReemplazoPaginasSwap(&paginaAux);
		}
		offsetPrevioHM = paginaAux->nroMarco*tamPagina + (offsetPrevioHM)%tamPagina; // sumo base mas offset
		memcpy(buffer + offsetBuffer,memoria + offsetPrevioHM,tam_heap_metadata - offsetBuffer);
		pthread_mutex_unlock(&mutex_memoria);

		memcpy(unHeapMetadata,buffer,tam_heap_metadata);

		free(buffer);

	}
	free(pagina);

	return unHeapMetadata;

}

/*
 * EXTRAS HM
 */

// offset previo al HM !!
bool existeHM(t_list * paginas, int offsetBuscado)
{
	pthread_mutex_lock(&mutex_segmento);
	int cantPaginas = list_size(paginas);
	pthread_mutex_unlock(&mutex_segmento);

	int tamMaximo = tamPagina * cantPaginas;
	t_pagina* unaPagina = obtenerPaginaAuxiliar(paginas, 0);

	int offset = unaPagina->nroMarco * tamPagina;
	free(unaPagina);

	int bytesLeidos = 0;
	t_heap_metadata* heapMetadata = malloc(tam_heap_metadata); // liberar
	int contador = 0;
	int bytesLeidosPagina = 0;

	while(tamMaximo - bytesLeidos > tam_heap_metadata)
	{
		if(offset == offsetBuscado)
		{
			free(heapMetadata);
			return true;
		}

		leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, paginas, &contador);
	}

	free(heapMetadata);

	return false;
}

/*
 * uint32_t obtenerPosicionPreviaHeap(t_list * paginas, int offset,int paginaActual) // agarra la ultima posicion de un heap y va a la primera posicion de este
{
	int tamanioPaginaUsado = offset % tamPagina;
	uint32_t posicionRetorno = 0;

	if(tamanioPaginaUsado >= tam_heap_metadata)
	{
		posicionRetorno = (offset - tam_heap_metadata); // ver!
	}
	else
	{
		// La idea es: Retrocedo una pagina, obtengo el siguiente marco contiguo (me paso un marco) y resto lo que me falto leer del HM

	int tamanioRestante = tam_heap_metadata - tamanioPaginaUsado;
	free(unaPagina);
	if(paginaActual==0)
		return 0;
	unaPagina = obtenerPaginaAuxiliar(paginas,paginaActual-1);
	posicionRetorno = tamPagina*(unaPagina->nroMarco+1)-tamanioRestante;
	}

	free(unaPagina);
	return posicionRetorno;
}
 *
 */



#endif /* MUSEHEAPMETADATA_H_ */
