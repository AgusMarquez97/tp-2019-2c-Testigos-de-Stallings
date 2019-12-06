/*
 * MuseHeapMetadata.h
 *
 *  Created on: Dec 5, 2019
 *      Author: agus
 */

#ifndef MUSEHEAPMETADATA_H_
#define MUSEHEAPMETADATA_H_

#include "MuseAuxiliares.h"

/*
 * LECTURA HM
 */

// La funcion que deberia llamar copy. Offset = primera posicion luego del HM en memoria!
int leerUnHeapMetadata(t_list * paginas, int offset, void ** buffer, int tamanio)
{
	if(existeHM(paginas, offset))
	{
		t_heap_metadata * unHeap = obtenerHeapMetadata(paginas, offset);
		if(unHeap->estaLibre)
			return HM_YA_LIBERADO;
		if(unHeap->offset < tamanio)
			return TAMANIO_SOBREPASADO;

		leerDatosHeap(paginas,offset+tam_heap_metadata,buffer,tamanio);
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

		if(!unHeap->estaLibre)
		{
			return escribirHeapMetadata(paginas, offset,-1);
		}
		return HM_YA_LIBERADO;

	}
	return HM_NO_EXISTENTE;
}

void leerDatosHeap(t_list * paginas, int offset, void ** buffer, int tamanio)
{
	int paginaActual = obtenerPaginaActual(paginas, offset);
	t_pagina * unaPagina = list_get(paginas,paginaActual);
	int bytesLeidos = 0;
	int bytesRestantesPagina = 0;

	while(bytesLeidos < tamanio)
	{
		bytesRestantesPagina = (tamPagina)*(unaPagina->nroMarco+1) - offset;

		pthread_mutex_lock(&mutex_memoria);
		memcpy(*buffer + bytesLeidos,memoria + offset,bytesRestantesPagina);
		pthread_mutex_unlock(&mutex_memoria);

		bytesLeidos += bytesRestantesPagina;

		unaPagina = list_get(paginas,(paginaActual+1));

		offset = unaPagina->nroMarco*tamPagina; // me paro en la primera posicion de la siguiente pagina

		if(tamanio - bytesLeidos < tamPagina)
			bytesRestantesPagina = tamanio - bytesLeidos;
		else
		bytesRestantesPagina = tamPagina;
	}
}

void leerHeapMetadata(t_heap_metadata** heapMetadata, int* bytesLeidos, int* bytesLeidosPagina, int* offset, t_list * paginas, int* nroPagina) {

	int bytesIniciales = *bytesLeidosPagina;
	int incremento = 0; // lo que me paso cuando paso de pagina
	int paginasSalteadas = 0;
	t_pagina* paginaDummy = malloc(sizeof(*paginaDummy));
	int sobrantePaginaInicial = tamPagina - (*bytesLeidosPagina);

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

	if(*bytesLeidosPagina > tamPagina) { // se paso de la pagina en la que estaba
		paginasSalteadas = cantidadPaginasPedidas(*bytesLeidosPagina);
		incremento = *bytesLeidosPagina - tamPagina*(paginasSalteadas);
		(*nroPagina) += paginasSalteadas;
		paginaDummy = list_get(paginas, (*nroPagina));
		*offset = paginaDummy->nroMarco * tamPagina + incremento;
		*bytesLeidosPagina = incremento;
	} else {
		*offset += incremento;
	}

	free(paginaDummy);
}

void leerHeapPartido(t_heap_metadata** heapMetadata, int* offset, int sobrante, int* nroPagina, t_list* paginas, t_pagina** paginaDummy) {

	void* buffer = malloc(tam_heap_metadata);
	pthread_mutex_lock(&mutex_memoria);
	memcpy(buffer, memoria + (*offset), sobrante);
	pthread_mutex_unlock(&mutex_memoria);

	(*nroPagina)++;

	(*paginaDummy) = list_get(paginas, (*nroPagina));
	*offset = (*paginaDummy)->nroMarco * tamPagina;

	pthread_mutex_lock(&mutex_memoria);
	memcpy(buffer, memoria + (*offset), tam_heap_metadata - sobrante);
	pthread_mutex_unlock(&mutex_memoria);

	memcpy(*heapMetadata, buffer, tam_heap_metadata);

	free(buffer);

}


/*
 * ESCRITURA HM
 */

// IMPORTANTE: ASUME QUE LAS PAGINAS YA SE RESERVARON CON AGREGAR PAGINAS

// El offset tiene que estar justo en el valor anterior al hm
int escribirHeapMetadata(t_list * listaPaginas, int offset, int tamanio)
{
		int paginaActual = obtenerPaginaActual(listaPaginas, offset);
		t_pagina * pagina = list_get(listaPaginas,paginaActual);
		t_heap_metadata * unHeapMetadata;

		int paginasPedidas;
		int bytesSobrantesUltimaPagina;
		int tamanioPaginaRestante;
		int tamanioOffsetHeap;

		int bytesEscritosPagina = 0;

		int bytesEscritos = 0;

		if(tamanio == -1) // free
		{
			unHeapMetadata = obtenerHeapMetadata(listaPaginas,offset); // validar OFFSET
			unHeapMetadata->estaLibre = true;
			tamanioOffsetHeap = unHeapMetadata->offset;
		}
		else
		{
			 // malloc normal
			tamanioOffsetHeap = tamanio;
			unHeapMetadata = malloc(tam_heap_metadata);
			unHeapMetadata->estaLibre = false;
			unHeapMetadata->offset = tamanio;
		}

		bytesSobrantesUltimaPagina = tamanioOffsetHeap;//int paginasPedidas = list_size(listaPaginas) - paginaActual - 1;
		paginasPedidas = cantidadPaginasPedidas(tamanioOffsetHeap) + 1;
		tamanioPaginaRestante = (tamPagina)*(pagina->nroMarco+1) - offset;

		if(tamanioPaginaRestante > tam_heap_metadata)
		{
			pthread_mutex_lock(&mutex_memoria);
			memcpy(memoria + offset,unHeapMetadata,tam_heap_metadata);
			pthread_mutex_unlock(&mutex_memoria);
		}
		else
		{
			void * buffer = malloc(tam_heap_metadata);
			int offsetBuffer = 0;

			memcpy(unHeapMetadata,buffer,tam_heap_metadata);

			pthread_mutex_lock(&mutex_memoria);
			memcpy(memoria + offset,buffer,tamanioPaginaRestante);
			pthread_mutex_unlock(&mutex_memoria);

			offsetBuffer+=tamanioPaginaRestante;
			pagina = list_get(listaPaginas,paginaActual+1);
			offset = pagina->nroMarco*tamPagina; //Obtengo la pagina siguiente

			pthread_mutex_lock(&mutex_memoria);
			memcpy(memoria + offset,buffer + offsetBuffer,tam_heap_metadata - offsetBuffer);
			pthread_mutex_unlock(&mutex_memoria);

			free(buffer);
		}

		bytesEscritos = tam_heap_metadata + unHeapMetadata->offset;

		if(bytesEscritos > tamanioPaginaRestante)
			bytesEscritosPagina = bytesEscritos - tamanioPaginaRestante;
		else
			bytesEscritosPagina = bytesEscritos;


		bytesSobrantesUltimaPagina = tamPagina*paginasPedidas - bytesEscritosPagina;

		if(bytesSobrantesUltimaPagina > tam_heap_metadata && tamanio != -1)
		{
			pagina = list_get(listaPaginas,list_size(listaPaginas) - 1);
			offset = pagina->nroMarco*tamPagina + (tamPagina - bytesSobrantesUltimaPagina);

			unHeapMetadata->estaLibre = true;
			unHeapMetadata->offset = bytesSobrantesUltimaPagina - tam_heap_metadata;

			pthread_mutex_lock(&mutex_memoria);
			memcpy(memoria + offset,unHeapMetadata,tam_heap_metadata); // siempre sera un heap entero
			pthread_mutex_unlock(&mutex_memoria);

			bytesEscritos += bytesSobrantesUltimaPagina;
		}

		free(unHeapMetadata);

		return bytesEscritos;
}

int escribirUnHeapMetadata(t_list * paginas, int offset, void ** buffer, int tamanio)
{
	if(existeHM(paginas, offset))
	{
		t_heap_metadata * unHeap = obtenerHeapMetadata(paginas, offset);
		if(unHeap->estaLibre)
			return HM_YA_LIBERADO;
		if(unHeap->offset < tamanio)
			return TAMANIO_SOBREPASADO;

		escribirDatosHeap(paginas,offset+tam_heap_metadata,buffer,tamanio);
		free(unHeap);
		return tamanio; // si llego nunca deberia fallar leerDatosHeap
	}
	return HM_NO_EXISTENTE;
}

void escribirDatosHeap(t_list * paginas, int offset, void ** buffer, int tamanio)
{
	int paginaActual = obtenerPaginaActual(paginas, offset);
	t_pagina * unaPagina = list_get(paginas,paginaActual);
	int bytesLeidos = 0;
	int bytesRestantesPagina = 0;

	while(bytesLeidos < tamanio)
	{
		bytesRestantesPagina = (tamPagina)*(unaPagina->nroMarco+1) - offset;

		pthread_mutex_lock(&mutex_memoria);
		memcpy(memoria + offset,*buffer + bytesLeidos,bytesRestantesPagina);
		pthread_mutex_unlock(&mutex_memoria);

		bytesLeidos += bytesRestantesPagina;

		unaPagina = list_get(paginas,(paginaActual+1));

		offset = unaPagina->nroMarco*tamPagina; // me paro en la primera posicion de la siguiente pagina

		if(tamanio - bytesLeidos < tamPagina)
			bytesRestantesPagina = tamanio - bytesLeidos;
		else
		bytesRestantesPagina = tamPagina;
	}
}

/*
 * Obtener HM
 */

t_heap_metadata * obtenerHeapMetadata(t_list * listaPaginas, int offset)
{
	t_heap_metadata * unHeapMetadata = malloc(tam_heap_metadata);
	t_pagina * pagina = obtenerPagina(listaPaginas,offset);

	int tamanioPaginaRestante = (tamPagina)*(pagina->nroMarco+1) - offset;

	if(tamanioPaginaRestante > tam_heap_metadata)
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
		pagina = list_get(listaPaginas,pagina->nroPagina+1);
		offset = pagina->nroMarco*tamPagina; //Obtengo la pagina siguiente

		pthread_mutex_lock(&mutex_memoria);
		memcpy(buffer + offsetBuffer,memoria + offset,tam_heap_metadata - offsetBuffer);
		pthread_mutex_unlock(&mutex_memoria);

		memcpy(unHeapMetadata,buffer,tam_heap_metadata);

		free(buffer);

	}

	return unHeapMetadata;

}

/*
 * EXTRAS HM
 */

int obtenerPosicionPreviaHeap(t_list * paginas, int offset) // agarra la ultima posicion de un heap y va a la primera posicion de este
{
	int paginaActual = obtenerPaginaActual(paginas,offset);
	t_pagina * unaPagina = list_get(paginas,paginaActual);
	int tamanioPaginaUsado = offset - (tamPagina)*(unaPagina->nroMarco);

	if(tamanioPaginaUsado >= tam_heap_metadata)
		return (offset - tam_heap_metadata);

		// La idea es: Retrocedo una pagina, obtengo el siguiente marco contiguo (me paso un marco) y resto lo que me falto leer del HM

	int tamanioRestante = tam_heap_metadata - tamanioPaginaUsado;
	unaPagina = list_get(paginas,paginaActual-1);

	return tamPagina*(unaPagina->nroMarco+1)-tamanioRestante;
}

// offset previo al HM !!
bool existeHM(t_list * paginas, int offsetBuscado)
{
	char msj[200];
	sprintf(msj,"cantidad de paginas %d",list_size(paginas));
	loggearInfo(msj);

	int cantPaginas = list_size(paginas);
	int tamMaximo = tamPagina * cantPaginas;
	t_pagina* unaPagina = list_get(paginas, 0);

	int offset = unaPagina->nroMarco * tamPagina;

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
