#ifndef MUSEAUXILIARES_H_
#define MUSEAUXILIARES_H_

#include "MUSE.h"

int obtenerCantidadMarcos(int tamanioPagina, int tamanioMemoria) {

	float tam_nec = (float)tamanioMemoria / tamanioPagina;
	if(tam_nec == (int)tam_nec) {
		return (int)tam_nec;
	}
	return (int)round(tam_nec + 0.5);

}

t_segmento* obtenerSegmento(t_list* segmentos, uint32_t posicionMemoria) {

	bool segmentoCorrespondiente(t_segmento* segmento) {
		return (posicionMemoria >= segmento->posicionInicial
				&& posicionMemoria <= (segmento->posicionInicial + 1) * segmento->tamanio);
	}
	return list_find(segmentos, (void*)segmentoCorrespondiente);

}

t_pagina* obtenerPagina(t_list* paginas, uint32_t posicionMemoria) {

	bool paginaCorrespondiente(t_pagina* pagina) {
		return (posicionMemoria >= pagina->nroPagina
				&& posicionMemoria <= (pagina->nroPagina + 1) * tamPagina);
	}
	return list_find(paginas, (void*)paginaCorrespondiente);

}

bool poseeSegmentos(char* idProceso) {

	return (dictionary_get(diccionarioProcesos, idProceso) != NULL);

}

void escribirPaginas(int cantidadMarcos, int tamanio, int primerMarco, int ultimoMarco) {

	int bytesReservados = cantidadMarcos * tamPagina;
	int bytesAEscribir = tam_heap_metadata + tamanio;
	int bytesSobrantes = bytesReservados - bytesAEscribir;
	int offset = primerMarco * tamPagina;
	char msj[300];

	t_heap_metadata* heapMetadata = malloc(tam_heap_metadata);

	heapMetadata->estaLibre = false;
	heapMetadata->offset = tamanio;

	pthread_mutex_lock(&mutex_memoria);
	memcpy(memoria + offset, heapMetadata, tam_heap_metadata);
	pthread_mutex_unlock(&mutex_memoria);

	sprintf(msj, "HeapMetadata:[offset: %d | estaLibre:No] en el marco %d", heapMetadata->offset, primerMarco);
	loggearInfo(msj);

	if(bytesSobrantes > tam_heap_metadata ) {
		offset = ultimoMarco * tamPagina + (tamPagina - bytesSobrantes);
		t_heap_metadata* heapMetadata = malloc(tam_heap_metadata);

		heapMetadata->estaLibre = true;
		heapMetadata->offset = bytesSobrantes - tam_heap_metadata;

		pthread_mutex_lock(&mutex_memoria);
		memcpy(memoria + offset, heapMetadata, tam_heap_metadata);
		pthread_mutex_unlock(&mutex_memoria);

		sprintf(msj, "HeapMetadata:[offset: %d | estaLibre:Si] en el marco %d", heapMetadata->offset, ultimoMarco);
		loggearInfo(msj);
	} else {
		sprintf(msj, "Fragmentación interna de %d bytes", bytesSobrantes);
	}

	free(heapMetadata);

}

void agregarPaginas(t_list** listaPaginas, int cantidadMarcos, int nroUltimaPagina) {

	t_pagina * pagina;

	for(int i = 0; i < cantidadMarcos; i++) {
		pagina = malloc(sizeof(pagina));

		pagina->nroMarco = asignarMarcoLibre(); // Agregar logica del algoritmo de reemplazo de pags
		pagina->nroPagina = i = nroUltimaPagina;

		list_add(*listaPaginas, pagina);
	}

}

t_list* obtenerPaginas(int tamanio, int cantidadMarcos) {

	t_list* listaPaginas = list_create();

	agregarPaginas(&listaPaginas,cantidadMarcos,0);

	int primerMarco = ((t_pagina*)list_get(listaPaginas, 0))->nroMarco;
	int ultimoMarco = ((t_pagina*)list_get(listaPaginas, cantidadMarcos - 1))->nroMarco;

	escribirPaginas(cantidadMarcos, tamanio, primerMarco, ultimoMarco);

	return listaPaginas;

}

t_segmento* instanciarSegmento(int tamanio, int cantidadFrames, int idSegmento, bool esCompartido, int posicionInicial) {

	t_list* listaPaginas = obtenerPaginas(tamanio, cantidadFrames);

	t_segmento * segmento = malloc(sizeof(t_segmento));

	segmento->id_segmento = idSegmento;
	segmento->esCompartido = esCompartido;
	segmento->posicionInicial = posicionInicial;
	segmento->tamanio = cantidadFrames * tamPagina;
	segmento->paginas = listaPaginas;

	return segmento;

}

void crearSegmento(char* idProceso, int tamanio, int cantidadMarcos, t_list* listaSegmentos, int idSegmento, bool esCompartido, int posicionInicial) {

		t_segmento* segmento = instanciarSegmento(tamanio, cantidadMarcos, idSegmento, esCompartido, posicionInicial);

		list_add(listaSegmentos, segmento);

		pthread_mutex_lock(&mutex_diccionario);
		dictionary_remove(diccionarioProcesos, idProceso);
		dictionary_put(diccionarioProcesos, idProceso, listaSegmentos);
		pthread_mutex_unlock(&mutex_diccionario);

}

void estirarSegmento(char* idProceso, t_segmento* segmento, int tamanio, int nuevaCantidadFrames, int offset, int sobrante) {

	t_list* listaPaginas = segmento->paginas;
	int nroUltimaPagina = list_size(listaPaginas);

	if(sobrante == 0)
		nroUltimaPagina = list_size(listaPaginas) + 1;

	agregarPaginas(&listaPaginas, nuevaCantidadFrames, nroUltimaPagina);
	//escribirHeapMetadata(listaPaginas,nroUltimaPagina,offset,tamanio,true); Escribir el heap nuevo considerando que se peuda partir CAMBIAR NOMBRE

	segmento->tamanio += nuevaCantidadFrames * tamPagina;

}

uint32_t completarSegmento(char * idProceso,t_segmento* segmento, int tamanio) {

	uint32_t posicionRetorno = 0;

	int cantPaginas = list_size(segmento->paginas);
	int tamMaximo = tamPagina * cantPaginas;
	t_pagina* unaPagina = list_get(segmento->paginas, 0);
	int offset = unaPagina->nroMarco * tamPagina;
	int bytesLeidos = 0;
	t_heap_metadata* heapMetadata = malloc(tam_heap_metadata); // liberar

	int sobrante = 0;

	int contador = 0;
	int bytesLeidosPagina = 0;

	leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, &segmento, &contador);

	while(tamMaximo - bytesLeidos > tam_heap_metadata) {
		if(heapMetadata->estaLibre && heapMetadata->offset >= tamanio) {
			escribirHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &segmento, &offset, &contador, &posicionRetorno);
			return posicionRetorno;
		}

		leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, &segmento, &contador);
	}

	sobrante = (tamMaximo - bytesLeidos);

	if(heapMetadata->estaLibre) {
		sobrante = heapMetadata->offset + tam_heap_metadata; // me sobro un hm entero
		offset -= (heapMetadata->offset + tam_heap_metadata);
	}

	int nuevaCantidadFrames = obtenerCantidadMarcos(tamPagina, tamanio + tam_heap_metadata - sobrante); // Frames necesarios para escribir en memoria

	estirarSegmento(idProceso, segmento, tamanio, nuevaCantidadFrames, offset, sobrante);

	return offset;

}

void leerHeapMetadata(t_heap_metadata** heapMetadata, int* bytesLeidos, int* bytesLeidosPagina, int* offset, t_segmento** segmento, int* nroPagina) {

	int bytesIniciales = *bytesLeidosPagina;
	int incremento = 0; // lo que me paso cuando paso de pagina
	int paginasSalteadas = 0;
	t_pagina* paginaDummy = malloc(sizeof(*paginaDummy));
	int sobrantePaginaInicial = tamPagina - (*bytesLeidosPagina);

	if(sobrantePaginaInicial<tam_heap_metadata) { // esta partido el proximo heap
		leerHeapPartido(heapMetadata, offset, sobrantePaginaInicial, nroPagina, segmento, &paginaDummy);
		incremento = (tam_heap_metadata - sobrantePaginaInicial) + (*heapMetadata)->offset;
		*bytesLeidosPagina = incremento;
		bytesIniciales = 0;
	} else {
		pthread_mutex_lock(&mutex_memoria);
		memcpy(*heapMetadata, memoria + (*offset), sizeof(t_heap_metadata));
		pthread_mutex_unlock(&mutex_memoria);
		*bytesLeidosPagina += sizeof(t_heap_metadata) + (*heapMetadata)->offset;
		incremento = *bytesLeidosPagina - bytesIniciales; // si paso de pagina => se va a recalcular
	}

	*bytesLeidos += sizeof(t_heap_metadata) + (*heapMetadata)->offset;

	if(*bytesLeidosPagina > tamPagina) { // se paso de la pagina en la que estaba
		paginasSalteadas = cantidadPaginasSalteadas(*bytesLeidosPagina);
		incremento = abs(tamPagina*paginasSalteadas - (*bytesLeidosPagina - bytesIniciales));// TESTEAR CON EXCEL. el abs es para cuando los bytes iniciales (hm partido) son 0, evita un if
		(*nroPagina) += paginasSalteadas;
		paginaDummy = list_get((*segmento)->paginas, (*nroPagina));
		*offset = paginaDummy->nroMarco * tamPagina + incremento;
		*bytesLeidosPagina = incremento;
	} else {
		*offset += incremento;
	}

	free(paginaDummy);

}

void leerHeapPartido(t_heap_metadata** heapMetadata, int* offset, int sobrante, int* nroPagina, t_segmento** segmento, t_pagina** paginaDummy) {

	void* buffer = malloc(tam_heap_metadata);
	pthread_mutex_lock(&mutex_memoria);
	memcpy(buffer, memoria + (*offset), sobrante);
	pthread_mutex_unlock(&mutex_memoria);

	(*nroPagina)++;

	(*paginaDummy) = list_get((*segmento)->paginas, (*nroPagina));
	*offset = (*paginaDummy)->nroMarco * tamPagina;

	pthread_mutex_lock(&mutex_memoria);
	memcpy(buffer, memoria + (*offset), tam_heap_metadata - sobrante);
	pthread_mutex_unlock(&mutex_memoria);

	memcpy(*heapMetadata, buffer, tam_heap_metadata);

	free(buffer);

}

void escribirHeapMetadata(t_heap_metadata** heapMetadata, int* bytesLeidos, int* bytesLeidosPagina, t_segmento** segmento, int* offset, int* nroPagina, uint32_t* posicionRetorno) {

}

int cantidadPaginasSalteadas(int offset) {

	return (int) offset / tamPagina; // redondea al menor numero para abajo

}

uint32_t analizarSegmento (char* idProceso, int tamanio, int cantidadFrames, bool esCompartido) {
	//Que pasa si en el medio de esta operacion el mismo proceso mediante otro hilo me inserta otro segmento al mismo tiempo = PROBLEMAS

	if(poseeSegmentos(idProceso)) {
		t_list * lista_segmentos = list_create();
		crearSegmento(idProceso, tamanio, cantidadFrames, lista_segmentos, 0, esCompartido, 0);
		return tam_heap_metadata;
	}

	uint32_t direccionRetorno;
	t_list* listaSegmentos;
	t_segmento* ultimoSegmento;
	uint32_t ultimaPosicionSegmento;

	pthread_mutex_lock(&mutex_diccionario);
	listaSegmentos = dictionary_get(diccionarioProcesos, idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	ultimoSegmento = list_get(listaSegmentos, list_size(listaSegmentos) - 1);
	ultimaPosicionSegmento = ultimoSegmento->posicionInicial + ultimoSegmento->tamanio;

	if(ultimoSegmento->esCompartido) {
		ultimaPosicionSegmento = ultimoSegmento->posicionInicial + ultimoSegmento->tamanio;
		crearSegmento(idProceso, tamanio, cantidadFrames, listaSegmentos, ultimoSegmento->id_segmento + 1, esCompartido, ultimaPosicionSegmento); // HACER
		direccionRetorno = ultimaPosicionSegmento + tam_heap_metadata;
	} else {
		direccionRetorno = completarSegmento(idProceso, ultimoSegmento, tamanio);
	}

	return direccionRetorno;

}

t_heap_metadata* obtenerHeapMetadata(int base, int offset) {

	t_heap_metadata* heapMetadata = malloc(tam_heap_metadata);

	pthread_mutex_lock(&mutex_memoria);
	memcpy(heapMetadata, memoria + base + offset - tam_heap_metadata, tam_heap_metadata);
	pthread_mutex_unlock(&mutex_memoria);

	return heapMetadata;

}

uint32_t liberarBytesMemoria(int base, int offset) {

	uint32_t bytesLiberados = 0;
	t_heap_metadata* heapMetadata = obtenerHeapMetadata(base, offset);

	heapMetadata->estaLibre = true;

	pthread_mutex_lock(&mutex_memoria);
	memcpy(memoria + base + offset - tam_heap_metadata, heapMetadata, tam_heap_metadata);
	pthread_mutex_unlock(&mutex_memoria);

	bytesLiberados = heapMetadata->offset;
	free(heapMetadata);

	return bytesLiberados;

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
		leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, &segmento, &contador);

		if(heapMetadata->estaLibre) {
			primerHeapMetadataLibre = offset - tam_heap_metadata;
			fragmentacion += bytesLeidos;
			leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, &segmento, &contador);

			while(heapMetadata->estaLibre) {
				fragmentacion += bytesLeidos;
				leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, &segmento, &contador);
			}

			heapMetadata->estaLibre = 1;
			heapMetadata->offset = fragmentacion;

			pthread_mutex_lock(&mutex_memoria);
			memcpy(memoria + primerHeapMetadataLibre, heapMetadata, tam_heap_metadata);
			pthread_mutex_unlock(&mutex_memoria);

			fragmentacion = 0;
		} else {
			leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, &segmento, &contador);
		}
	}

	free(heapMetadata);

}

void* leerDeMemoria(int posicionInicial, int tamanio) {

	void* buffer = malloc(tamanio);
	pthread_mutex_lock(&mutex_memoria);
	memcpy(buffer, memoria + posicionInicial, tamanio);
	pthread_mutex_unlock(&mutex_memoria);

	return buffer;

}

void escribirEnMemoria(void* contenido, int posicionInicial, int tamanio) {

	pthread_mutex_lock(&mutex_memoria);
	memcpy(memoria + posicionInicial, contenido, tamanio);
	pthread_mutex_unlock(&mutex_memoria);

}

void liberarMarcoBitarray(int nroMarco) {

	pthread_mutex_lock(&mutex_marcos_libres);
	bitarray_clean_bit(marcosMemoriaPrincipal, nroMarco);
	pthread_mutex_unlock(&mutex_marcos_libres);

}

bool estaLibreMarco(int nroMarco) {

	return bitarray_test_bit(marcosMemoriaPrincipal, nroMarco) == 0;

}

int asignarMarcoLibre() {

	for(int i = 0; i < cantidadMarcosMemoriaPrincipal; i++) {
		if(estaLibreMarco(i)) {
			pthread_mutex_lock(&mutex_marcos_libres);
			bitarray_set_bit(marcosMemoriaPrincipal, i);
			pthread_mutex_unlock(&mutex_marcos_libres);
			return i;
		}
	}

	// Si sale del bucle, llamar al algoritmo de reemplazo

	return -1;

}

#endif /* MUSEAUXILIARES_H_ */

/*
int buscarPosicionHeapCandidato(t_segmento* segmento, int bytesPedidos) {

	int cantPaginas = list_size(segmento->paginas);
	int tamMaximo = tamPagina * cantPaginas;
	t_pagina* pagina = list_get(segmento->paginas, 0);
	t_heap_metadata* heapMetadata = malloc(tam_heap_metadata));
	int bytesLeidos = 0;
	int bytesLeidosPagina = 0;
	int offset = pagina->nroMarco * tamPagina;
	int contador = 0;

	leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, &segmento, &contador);

	while((tamMaximo - bytesLeidos) >= tam_heap_metadata) {

		if(heapMetadata->estaLibre && heapMetadata->offset >= bytesPedidos) {
			return offset - tam_heap_metadata;
		}

		leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, &segmento, &contador);

	}

	while((!heapMetadata->estaLibre && !ultimaPagina) || (tamMaximo - bytesLeidos) > tam_heap_metadata)
	{
				if(heapMetadata->estaLibre && heapMetadata->offset <= tamanio)
				{
					escribirHeapMetadata(&heapMetadata,&bytesLeidos,&bytesLeidosPagina,&segmento,&offset,&contador,&posicionRetorno);
					return posicionRetorno;
				}

				leerHeapMetadata(&heapMetadata,&bytesLeidos,&bytesLeidosPagina, &offset,&segmento,&contador);

				if(contador == cantPaginas) // REVISAR OPERACION


	int sobrante = tamMaximo - bytesLeidos;

	if(sobrante >= tam_heap_metadata) {
		return offset - (tam_heap_metadata - heapMetadata->offset);
	}

	agregarHeapMetadata(segmento, sobrante);

	return heapMetadata;

}


void liberarMarcos()
{
	for(int i = 0; i < cantidadMarcosMemoriaPrincipal;i++)
	{
		liberarMarco(i);
	}
}

void liberarMarco(int nroMarco)
{
	int offset = nroMarco*tamPagina;
	memset(tamMemoria + offset,0,tamPagina);

	pthread_mutex_lock(&mutex_marcos_libres);
	bitarray_clean_bit(marcosMemoriaPrincipal,nroMarco); // Se libera el marco para poder ser usado
	pthread_mutex_unlock(&mutex_marcos_libres);

}

bool estaLibre(t_bitarray * unBitArray,int nroMarco)
{
	return bitarray_test_bit(unBitArray,nroMarco)==0;
}

bool estaLlena(t_bitarray * unBitArray)
{
	for(int i = 0; i < cantidadMarcosMemoriaPrincipal;i++)
		{
			if(estaLibre(i))
			{
				return false;
			}
		}
	return true;
}

*/
