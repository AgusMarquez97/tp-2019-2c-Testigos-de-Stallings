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

// La funcion que deberia llamar copy. Offset = primera posicion luego del HM en memoria!
int leerUnHeapMetadata(t_list * paginas, int posicionAnteriorHeap,int posicionPosteriorHeap, void ** buffer, int tamanio)
{
	if(existeHM(paginas, posicionAnteriorHeap))
	{
		t_heap_metadata * unHeap = obtenerHeapMetadata(paginas, posicionAnteriorHeap);
		if(unHeap->estaLibre)
			return HM_YA_LIBERADO;
		if(unHeap->offset < tamanio)
			return TAMANIO_SOBREPASADO;

		leerDatosHeap(paginas,posicionPosteriorHeap,buffer,tamanio);
		free(unHeap);
		return tamanio; // si llego nunca deberia fallar leerDatosHeap
	}
	return HM_NO_EXISTENTE;
}

int liberarUnHeapMetadata(t_list * paginas, int offset)
{
	if(existeHM(paginas, offset))
	{
		t_heap_metadata * unHeap = obtenerHeapMetadata(paginas, offset);

		int nroPagina = obtenerPaginaActual(paginas, offset);
		t_pagina* paginaAuxiliar = obtenerPaginaAuxiliar(paginas, nroPagina);
		int marcoSiguiente = paginaAuxiliar->nroMarco + 1;
		free(paginaAuxiliar);
		int tamanioPaginaRestante = (tamPagina)*(marcoSiguiente) - offset;

		if(!unHeap->estaLibre)
		{
			unHeap->estaLibre = true;
			return escribirUnHeapMetadata(paginas, nroPagina, unHeap, &offset, tamanioPaginaRestante);
		}
		return HM_YA_LIBERADO;

	}
	return HM_NO_EXISTENTE;
}

void leerDatosHeap(t_list * paginas, int posicionPosteriorHeap, void ** buffer, int tamanio)
{

		int paginaActual = obtenerPaginaActual(paginas, posicionPosteriorHeap);
		t_pagina * unaPagina = obtenerPaginaAuxiliar(paginas,paginaActual);
		int bytesLeidos = 0;
		int bytesRestantesPagina = 0;

		if(tamanio > tamPagina)
			bytesRestantesPagina = (tamPagina)*(unaPagina->nroMarco+1) - posicionPosteriorHeap;
		else
			bytesRestantesPagina = tamanio;

		while(bytesLeidos < tamanio)
		{
			pthread_mutex_lock(&mutex_memoria);
			memcpy(*buffer + bytesLeidos,memoria + posicionPosteriorHeap,bytesRestantesPagina);
			pthread_mutex_unlock(&mutex_memoria);

			bytesLeidos += bytesRestantesPagina;

			if(bytesLeidos == tamanio)
				break;

			free(unaPagina);
			paginaActual++;

			t_pagina* pagina_auxiliar = list_get(paginas,paginaActual); // ver posibles problemas de concurrencia!
			if(!estaEnMemoria(paginas,paginaActual))
				rutinaReemplazoPaginasSwap(&pagina_auxiliar);

			unaPagina = obtenerPaginaAuxiliar(paginas,paginaActual);

			posicionPosteriorHeap = unaPagina->nroMarco*tamPagina; // me paro en la primera posicion de la siguiente pagina

			if(tamanio - bytesLeidos < tamPagina)
				bytesRestantesPagina = tamanio - bytesLeidos;
			else
				bytesRestantesPagina = tamPagina;
		}
		free(unaPagina);
}

void leerHeapMetadata(t_heap_metadata** heapMetadata, int* bytesLeidos, int* bytesLeidosPagina, int* offset, t_list * paginas, int* nroPagina) {

	int bytesIniciales = *bytesLeidosPagina;
	int incremento = 0; // lo que me paso cuando paso de pagina
	int paginasSalteadas = 0;
	t_pagina* paginaDummy = malloc(sizeof(*paginaDummy));
	int sobrantePaginaInicial = tamPagina - (*bytesLeidosPagina);
	int nroMarco = 0;

	if(sobrantePaginaInicial<tam_heap_metadata) { // esta partido el proximo heap
		leerHeapPartido(heapMetadata, offset, sobrantePaginaInicial, nroPagina, paginas, &paginaDummy);
		incremento = (tam_heap_metadata - sobrantePaginaInicial) + (*heapMetadata)->offset;
		*bytesLeidosPagina = incremento;
		bytesIniciales = 0;
	} else {
		pthread_mutex_lock(&mutex_memoria);
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

		if(!estaEnMemoria(paginas,(*nroPagina)))
		{
			t_pagina * paginaAux = list_get(paginas,(*nroPagina));
			rutinaReemplazoPaginasSwap(&paginaAux);
		}

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

void leerHeapPartido(t_heap_metadata** heapMetadata, int* offset, int sobrante, int* nroPagina, t_list* paginas, t_pagina** paginaDummy) {

	void* buffer = malloc(tam_heap_metadata);

	pthread_mutex_lock(&mutex_memoria);
	memcpy(buffer, memoria + (*offset), sobrante);
	pthread_mutex_unlock(&mutex_memoria);

	(*nroPagina)++;

	if(!estaEnMemoria(paginas,*nroPagina))
	{
		t_pagina * paginaAux = list_get(paginas,*nroPagina);
		rutinaReemplazoPaginasSwap(&paginaAux);
	}

	free(*paginaDummy);

	(*paginaDummy) = obtenerPaginaAuxiliar(paginas, (*nroPagina));
	*offset = (*paginaDummy)->nroMarco * tamPagina;

	pthread_mutex_lock(&mutex_memoria);
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

	if(tamanioPaginaRestante >= tam_heap_metadata)
	{
		pthread_mutex_lock(&mutex_memoria);
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
		memcpy(memoria + *offset,buffer,tamanioPaginaRestante);
		pthread_mutex_unlock(&mutex_memoria);

		offsetBuffer+=tamanioPaginaRestante;
		paginaActual++;

		if(!estaEnMemoria(listaPaginas,paginaActual))
		{
			t_pagina * paginaAux = list_get(listaPaginas,paginaActual);
			rutinaReemplazoPaginasSwap(&paginaAux);
		}

		t_pagina * pagina = obtenerPaginaAuxiliar(listaPaginas,paginaActual);

		*offset = pagina->nroMarco*tamPagina; //Obtengo la pagina siguiente

		pthread_mutex_lock(&mutex_memoria);
		memcpy(memoria + *offset,buffer + offsetBuffer,tam_heap_metadata - offsetBuffer);
		pthread_mutex_unlock(&mutex_memoria);

		*offset += (tam_heap_metadata - offsetBuffer);

		free(buffer);
		free(pagina);
	}

	return tam_heap_metadata + unHeapMetadata->offset;
}


// El offset tiene que estar justo en el valor anterior al hm
int escribirHeapMetadata(t_list * listaPaginas, int offset, int tamanio, int offsetMaximo)
{
		int paginaActual = obtenerPaginaActual(listaPaginas, offset);

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
			if(!estaEnMemoria(listaPaginas,paginaActual + paginasPedidas))
			{
				t_pagina * paginaAux = list_get(listaPaginas,paginaActual + paginasPedidas);
				rutinaReemplazoPaginasSwap(&paginaAux);
			}

			t_pagina * pagina = obtenerPaginaAuxiliar(listaPaginas,paginaActual + paginasPedidas); // La idea es ir siempre a la ultima pagina donde este el heap

			offset = pagina->nroMarco*tamPagina + (tamPagina - bytesSobrantesUltimaPagina);

			unHeapMetadata->estaLibre = true;

			if(offsetMaximo!=0)
			{
				offset = pagina->nroMarco*tamPagina + (offsetMaximo - bytesSobrantesUltimaPagina) + posicionActual%tamPagina;
				unHeapMetadata->offset = (offsetMaximo - bytesEscritos) - tam_heap_metadata; // PUEDE ESTAR PARTIDO => ANDA BUSCANDO LA 9mm
			}
			else
				unHeapMetadata->offset = bytesSobrantesUltimaPagina - tam_heap_metadata; // PUEDE ESTAR PARTIDO => ANDA BUSCANDO LA 9mm

			escribirUnHeapMetadata(listaPaginas,pagina->nroPagina,unHeapMetadata, &offset, bytesSobrantesUltimaPagina);
			bytesEscritos += bytesSobrantesUltimaPagina;
		}

		free(unHeapMetadata);

		return bytesEscritos;
}

// funcion para cpy => escribe datos despues del heap
int escribirDatosHeapMetadata(t_list * paginas, int posicionAnteriorHeap,int posicionPosteriorHeap, void ** buffer, int tamanio)
{
	if(existeHM(paginas, posicionAnteriorHeap))
	{
		t_heap_metadata * unHeap = obtenerHeapMetadata(paginas, posicionAnteriorHeap);
		if(unHeap->estaLibre)
			return HM_YA_LIBERADO;
		if(unHeap->offset < tamanio) // Para hacer esto infalible => pasar el segmento, to do para evitar el caso de memoria compartida (NO LO PIDEN)
			return TAMANIO_SOBREPASADO;

		escribirDatosHeap(paginas,posicionPosteriorHeap,buffer,tamanio);
		free(unHeap);
		return tamanio; // si llego nunca deberia fallar leerDatosHeap
	}
	return HM_NO_EXISTENTE;
}

void escribirDatosHeap(t_list * paginas, int posicionPosteriorHeap, void ** buffer, int tamanio)
{
	int paginaActual = obtenerPaginaActual(paginas, posicionPosteriorHeap);
	t_pagina * unaPagina = obtenerPaginaAuxiliar(paginas,paginaActual);
	int bytesEscritos = 0;
	int bytesRestantesPagina = 0;

	if(tamanio > tamPagina)
		bytesRestantesPagina = tamPagina - posicionPosteriorHeap%tamPagina;
	else
		bytesRestantesPagina = tamanio;

	while(bytesEscritos < tamanio)
	{

		pthread_mutex_lock(&mutex_memoria);
		memcpy(memoria + posicionPosteriorHeap,*buffer + bytesEscritos,bytesRestantesPagina);
		pthread_mutex_unlock(&mutex_memoria);

		bytesEscritos += bytesRestantesPagina;

		if(bytesEscritos == tamanio)
			break;

		free(unaPagina);
		paginaActual++;

		if(!estaEnMemoria(paginas,paginaActual))
		{
			t_pagina * paginaAux = list_get(paginas,paginaActual);
			rutinaReemplazoPaginasSwap(&paginaAux);
		}

		unaPagina = obtenerPaginaAuxiliar(paginas,paginaActual);

		posicionPosteriorHeap = unaPagina->nroMarco*tamPagina; // me paro en la primera posicion de la siguiente pagina

		if(tamanio - bytesEscritos < tamPagina)
			bytesRestantesPagina = tamanio - bytesEscritos;
		else
			bytesRestantesPagina = tamPagina;
	}
	free(unaPagina);
}

/*
 * Obtener HM
 */

t_heap_metadata * obtenerHeapMetadata(t_list * listaPaginas, int offset)
{
	t_heap_metadata * unHeapMetadata = malloc(tam_heap_metadata);

	int nroPagina = obtenerPaginaActual(listaPaginas,offset);
	t_pagina * pagina = obtenerPaginaAuxiliar(listaPaginas, nroPagina);

	int tamanioPaginaRestante = (tamPagina)*(pagina->nroMarco+1) - offset;

	if(tamanioPaginaRestante >= tam_heap_metadata)
	{

		pthread_mutex_lock(&mutex_memoria);
		memcpy(unHeapMetadata,memoria + offset,tam_heap_metadata);
		pthread_mutex_unlock(&mutex_memoria);

	}
	else
	{
		void * buffer = malloc(tam_heap_metadata);
		int offsetBuffer = 0;

		pthread_mutex_lock(&mutex_memoria);
		memcpy(buffer,memoria + offset,tamanioPaginaRestante);
		pthread_mutex_unlock(&mutex_memoria);

		offsetBuffer+=tamanioPaginaRestante;
		free(pagina);

		if(!estaEnMemoria(listaPaginas,nroPagina+1))
		{
			t_pagina * paginaAux = list_get(listaPaginas,nroPagina+1);
			rutinaReemplazoPaginasSwap(&paginaAux);
		}

		pagina = obtenerPaginaAuxiliar(listaPaginas,nroPagina+1);
		offset = pagina->nroMarco*tamPagina; //Obtengo la pagina siguiente

		pthread_mutex_lock(&mutex_memoria);
		memcpy(buffer + offsetBuffer,memoria + offset,tam_heap_metadata - offsetBuffer);
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

uint32_t obtenerPosicionPreviaHeap(t_list * paginas, int offset) // agarra la ultima posicion de un heap y va a la primera posicion de este
{
	int paginaActual = obtenerPaginaActual(paginas,offset);
	t_pagina * unaPagina = obtenerPaginaAuxiliar(paginas,paginaActual);
	int tamanioPaginaUsado = offset - (tamPagina)*(unaPagina->nroMarco);
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
	unaPagina = obtenerPaginaAuxiliar(paginas,paginaActual-1);
	posicionRetorno = tamPagina*(unaPagina->nroMarco+1)-tamanioRestante;
	}

	free(unaPagina);
	return posicionRetorno;
}

// offset previo al HM !!
bool existeHM(t_list * paginas, int offsetBuscado)
{
	int cantPaginas = list_size(paginas);
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
			return true;

		leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, paginas, &contador);
	}



	return false;
}



#endif /* MUSEHEAPMETADATA_H_ */
