#ifndef MUSEAUXILIARES_H_
#define MUSEAUXILIARES_H_

#include "MUSE.h"

bool existeEnElDiccionario(char* idProceso) {

	bool retorno = false;

	pthread_mutex_lock(&mutex_diccionario);

	if(diccionarioProcesos) {
		if(!dictionary_is_empty(diccionarioProcesos)) {
			if(dictionary_has_key(diccionarioProcesos, idProceso)) {
				retorno = true;
			}
		}
	}
	pthread_mutex_unlock(&mutex_diccionario);

	return retorno;
}

int obtenerCantidadMarcos(int tamanioPagina, int tamanioMemoria) {

	float tam_nec = (float)tamanioMemoria / tamanioPagina;
	if(tam_nec == (int)tam_nec) {
		return (int)tam_nec;
	}
	return (int)round(tam_nec + 0.5);

}

t_segmento* obtenerUnSegmento(char * idProceso, uint32_t posicionMemoria)
{

	pthread_mutex_lock(&mutex_diccionario);
	t_list * segmentos = dictionary_get(diccionarioProcesos, idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	return obtenerSegmento(segmentos,posicionMemoria);
}


t_segmento* obtenerSegmento(t_list* segmentos, uint32_t posicionMemoria) {

	bool segmentoCorrespondiente(t_segmento* segmento) {
		return (posicionMemoria >= segmento->posicionInicial
				&& posicionMemoria <= (segmento->posicionInicial + segmento->tamanio));
	}
	return list_find(segmentos, (void*)segmentoCorrespondiente);
}

t_pagina* obtenerPagina(t_list* paginas, uint32_t posicionSegmento) {

	bool paginaCorrespondiente(t_pagina* pagina) {
		return (posicionSegmento >= pagina->nroPagina*tamPagina
				&& posicionSegmento <= (pagina->nroPagina + 1) * tamPagina);
	}
	return list_find(paginas, (void*)paginaCorrespondiente);

}

bool poseeSegmentos(char* idProceso) {

	return (dictionary_get(diccionarioProcesos, idProceso) != NULL);

}


int cantidadPaginasPedidas(int offset) {

	return (int) offset / tamPagina; // redondea al menor numero para abajo

}

void* leerDeMemoria(int posicionInicial, int tamanio) {

	void* buffer = malloc(tamanio);
	memcpy(buffer, memoria + posicionInicial, tamanio); // no hacen falta los locks, ya estan puestos antes

	return buffer;

}

void escribirEnMemoria(void* contenido, int posicionInicial, int tamanio) {

	memcpy(memoria + posicionInicial, contenido, tamanio);

}

void liberarMarcoBitarray(int nroMarco) {

	pthread_mutex_lock(&mutex_marcos_libres);
	bitarray_clean_bit(marcosMemoriaPrincipal, nroMarco);
	pthread_mutex_unlock(&mutex_marcos_libres);

}

bool estaLibreMarco(int nroMarco) {
	pthread_mutex_lock(&mutex_marcos_libres);
	bool retorno = bitarray_test_bit(marcosMemoriaPrincipal, nroMarco) == 0;
	pthread_mutex_unlock(&mutex_marcos_libres);
	return retorno;

}


int obtenerPaginaActual(t_list * paginas, int offset)
{
	for(int i = 0;i < list_size(paginas);i++)
	{
	t_pagina * unaPagina = list_get(paginas,i);
	if(unaPagina->nroMarco == (int) offset / tamPagina)
		return i;
	}
	return 0;
}

//retorna la primera posicion LUEGO del HM (o eso deberia hacer)

uint32_t obtenerDireccionMemoria(t_list* listaPaginas,uint32_t posicionSegmento)
{

	uint32_t offset = 0;

	t_pagina* paginaBuscada = obtenerPagina(listaPaginas, posicionSegmento);

	offset = posicionSegmento - tamPagina * paginaBuscada->nroPagina;

	return paginaBuscada->nroMarco*tamPagina + offset; // base + offset
}

t_segmento * buscarSegmento(t_list * segmentos, uint32_t posicionSegmento)
{
	bool encontrarSegmento(t_segmento * unSegmento)
	{
		return (unSegmento->posicionInicial >= posicionSegmento && unSegmento->posicionInicial*unSegmento->tamanio <= posicionSegmento);
	}
	return list_find(segmentos,(void*)encontrarSegmento);
}

t_pagina * obtenerPaginaAuxiliar(t_list * paginas, int nroPagina)
{
	t_pagina * paginaAuxiliar = malloc(sizeof(*paginaAuxiliar));

	t_pagina * paginaReal = list_get(paginas,nroPagina);

	memcpy(paginaAuxiliar,paginaReal,sizeof(t_pagina));

	return paginaAuxiliar;

}


t_list * obtenerPaginas(char* idProceso, uint32_t posicionSegmento)
{
	t_list * segmentos;
	t_segmento* segmento;

	pthread_mutex_lock(&mutex_diccionario);
	segmentos = dictionary_get(diccionarioProcesos,idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	segmento = obtenerSegmento(segmentos, posicionSegmento); // ver de hacer validacion por el nulo

	if(segmento)
		return segmento->paginas;
	return NULL;

}

void usarPagina(t_list * paginas, int nroPagina)
{
	t_pagina * unaPagina = list_get(paginas,nroPagina);
	unaPagina->uso=1;
}


#endif /* MUSEAUXILIARES_H_ */
