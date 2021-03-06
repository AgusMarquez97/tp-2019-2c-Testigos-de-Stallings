#ifndef MUSEMALLOC_H_
#define MUSEMALLOC_H_

#include "MuseAuxiliares.h"

uint32_t analizarSegmento (char* idProceso, int tamanio, int cantidadFrames, bool esCompartido) {
	//Que pasa si en el medio de esta operacion el mismo proceso mediante otro hilo me inserta
	//otro segmento al mismo tiempo [!!]

	uint32_t direccionRetorno = 0;
	t_list* listaSegmentos;
	int nroSegmento = 0;

	if(!poseeSegmentos(idProceso)) {
		listaSegmentos = list_create();
		crearSegmento(idProceso, tamanio, cantidadFrames, listaSegmentos, 0, esCompartido, 0);
		direccionRetorno =  tam_heap_metadata;
	} else {
		t_segmento* ultimoSegmento;
		uint32_t ultimaPosicionSegmento;

		pthread_mutex_lock(&mutex_diccionario);
		listaSegmentos = dictionary_get(diccionarioProcesos, idProceso);
		pthread_mutex_lock(&mutex_segmento);

		ultimoSegmento = list_get(listaSegmentos, list_size(listaSegmentos) - 1);
		ultimaPosicionSegmento = ultimoSegmento->posicionInicial + ultimoSegmento->tamanio;
		nroSegmento = ultimoSegmento->id_segmento + 1;

		pthread_mutex_unlock(&mutex_segmento);
		pthread_mutex_unlock(&mutex_diccionario);

		if(ultimoSegmento->esCompartido || esCompartido) {
			crearSegmento(idProceso, tamanio, cantidadFrames, listaSegmentos, nroSegmento, esCompartido, ultimaPosicionSegmento);
			direccionRetorno = ultimaPosicionSegmento + tam_heap_metadata;
		} else {
			direccionRetorno = completarSegmento(idProceso, ultimoSegmento, tamanio);
		}

	}

	return direccionRetorno;

}


void crearSegmento(char* idProceso, int tamanio, int cantidadMarcos, t_list* listaSegmentos, int idSegmento, bool esCompartido, int posicionInicial) {

		t_segmento* segmento = instanciarSegmento(tamanio, cantidadMarcos, idSegmento, esCompartido, posicionInicial);

		list_add(listaSegmentos, segmento);

		pthread_mutex_lock(&mutex_diccionario);
		dictionary_remove(diccionarioProcesos, idProceso);
		dictionary_put(diccionarioProcesos, idProceso, listaSegmentos);
		pthread_mutex_unlock(&mutex_diccionario);

}

t_segmento* instanciarSegmento(int tamanio, int cantidadFrames, int idSegmento, bool esCompartido, int posicionInicial) {

	t_list* listaPaginas = crearListaPaginas(tamanio, cantidadFrames, esCompartido);

	t_segmento* segmento = malloc(sizeof(t_segmento));

	segmento->id_segmento = idSegmento;
	segmento->esCompartido = esCompartido;
	segmento->posicionInicial = posicionInicial;
	segmento->tamanio = cantidadFrames * tamPagina;
	segmento->paginas = listaPaginas;
	segmento->archivo = NULL;

	return segmento;

}

t_list* crearListaPaginas(int tamanio, int cantidadMarcos, bool esCompartido) {

	t_list* listaPaginas = list_create();

	agregarPaginas(&listaPaginas, cantidadMarcos, 0, esCompartido);

	int primerMarco = ((t_pagina*)list_get(listaPaginas, 0))->nroMarco; // ver si en el medio

	escribirHeapMetadata(listaPaginas, 0, primerMarco * tamPagina, tamanio, false);

	return listaPaginas;

}

void agregarPaginas(t_list** listaPaginas, int cantidadMarcos, int nroUltimaPagina, bool esCompartido) {
	// Analizar de agregar log de cada pagina que se pide por proceso

	t_pagina* pagina;

	for(int i = 0; i < cantidadMarcos; i++) {

		pagina = malloc(sizeof(*pagina));

		pagina->nroMarco = asignarMarcoLibre(); // Agregar logica del algoritmo de reemplazo de pags
		pagina->nroPagina = i + nroUltimaPagina;
		pagina->nroPaginaSwap = -1;
		pagina->uso = 1;
		pagina->modificada = 0;

		pthread_mutex_lock(&mutex_segmento);
		list_add(*listaPaginas, pagina);
		pthread_mutex_unlock(&mutex_segmento);

		pthread_mutex_lock(&mutex_algoritmo_reemplazo);
		list_add(listaPaginasClockModificado, pagina);
		pthread_mutex_unlock(&mutex_algoritmo_reemplazo);

	}

}

uint32_t completarSegmento(char * idProceso, t_segmento* segmento, int tamanio) {

	pthread_mutex_lock(&mutex_segmento);
	int cantPaginas = list_size(segmento->paginas);
	int tamMaximo = tamPagina * cantPaginas;
	t_pagina* unaPagina = list_get(segmento->paginas, 0);
	pthread_mutex_unlock(&mutex_segmento);

	int offset = unaPagina->nroMarco * tamPagina;
	int bytesLeidos = 0;

	t_heap_metadata* heapMetadata = malloc(tam_heap_metadata); // liberar

	int sobrante = 0;

	int contador = 0;
	int bytesLeidosPagina = 0;
	int offsetAnterior = 0;
	int auxBytesLeidos;
	int paginaAnterior;


	while(tamMaximo - bytesLeidos > tam_heap_metadata) {

		offsetAnterior = offset;
		auxBytesLeidos = bytesLeidos;
		paginaAnterior = contador;
		leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset,segmento->paginas, &contador);

		if(heapMetadata->estaLibre && heapMetadata->offset >= (tamanio + tam_heap_metadata)) {

			bool tieneUnoSiguiente = existeHM(segmento->paginas, offset);

			if(tieneUnoSiguiente)
				escribirHeapMetadata(segmento->paginas, paginaAnterior, offsetAnterior, tamanio, tam_heap_metadata + heapMetadata->offset); // validado
			else
				escribirHeapMetadata(segmento->paginas, paginaAnterior, offsetAnterior, tamanio, false);

			free(heapMetadata);
			return segmento->posicionInicial + auxBytesLeidos + tam_heap_metadata;

		}

	}

	int baseSegmento = bytesLeidos;

	sobrante = (tamMaximo - bytesLeidos);

	if(heapMetadata->estaLibre) {
		sobrante = heapMetadata->offset + tam_heap_metadata; // me sobro un hm entero
		offset = offsetAnterior; // Pongo el offset en la primera posicion libre
		baseSegmento -= sobrante; // ver!
		contador = paginaAnterior;
	}

	int nuevaCantidadFrames = obtenerCantidadMarcos(tamPagina, tamanio + tam_heap_metadata - sobrante); // Frames necesarios para escribir en memoria

	int retorno = estirarSegmento(baseSegmento, idProceso, segmento, tamanio, nuevaCantidadFrames, offset, sobrante, contador);

	free(heapMetadata); // testear!!

	return segmento->posicionInicial + retorno;

}

// offset anterior al heap!
int estirarSegmento(int baseSegmento, char* idProceso, t_segmento* segmento, int tamanio, int nuevaCantidadFrames, int offset, int sobrante, int paginaActual) {

	// seguramente aca necesite un mutex => nadie puede acceder a sus paginas temporalmente
	pthread_mutex_lock(&mutex_segmento);
	t_list* listaPaginas = segmento->paginas;
	int nroUltimaPagina = list_size(listaPaginas);
	pthread_mutex_unlock(&mutex_segmento);
	agregarPaginas(&listaPaginas, nuevaCantidadFrames, nroUltimaPagina, false);
	pthread_mutex_lock(&mutex_segmento);
	segmento->tamanio = list_size(listaPaginas) * tamPagina; // ver si es necesario un mutex por cada operacion con el segmento
	pthread_mutex_unlock(&mutex_segmento);

	// hasta aca
	escribirHeapMetadata(listaPaginas, paginaActual, offset, tamanio, false); // Escribir el heap nuevo en memoria. Considera heap partido

	return baseSegmento + tam_heap_metadata;
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
	pthread_mutex_lock(&mutex_memoria);
	moverMarcosASwap();
	pthread_mutex_unlock(&mutex_memoria);

	return asignarMarcoLibre();

}

#endif /* MUSEMALLOC_H_ */
