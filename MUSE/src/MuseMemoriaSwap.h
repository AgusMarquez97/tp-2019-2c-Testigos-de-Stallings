/*
 * MuseMemoriaSwap.h
 *
 *  Created on: Dec 14, 2019
 *      Author: agus
 */

#ifndef MUSEMEMORIASWAP_H_
#define MUSEMEMORIASWAP_H_

#include "MuseMemoriaCompartida.h"

/*
 * Rutina para mover marcos de MP -> MS
 * => Se ejecuta cuando no encuentro marcos libres de la memoria principal, en otras palabras, cuando pido mas marcos (malloc y mmap)
 */
void moverMarcosASwap()
{
	pthread_mutex_lock(&mutex_algoritmo_reemplazo);
	t_pagina * paginaReemplazo = ejecutarAlgoritmoReemplazo();
	pthread_mutex_unlock(&mutex_algoritmo_reemplazo);

	reemplazarVictima(&paginaReemplazo, false);
}

/*
 * Rutina para obtener una pagina de la memoria swap.
 * => Se ejecuta cuando intento leer una pagina que no esta en memoria (Todas las operacion excepto handshake y unmap)
 */
void rutinaReemplazoPaginasSwap(t_pagina** unaPagina)
{
	pthread_mutex_lock(&mutex_algoritmo_reemplazo);
	t_pagina * paginaVictima = ejecutarAlgoritmoReemplazo(); // obtengo la pagina que quiero reemplazar
	pthread_mutex_unlock(&mutex_algoritmo_reemplazo);

	int marcoVictima = paginaVictima->nroMarco;

	/*
	 * Parte del reemplazo de la victima
	 */

	reemplazarVictima(&paginaVictima,true);

	/*
	 * Parte del recupero de la pagina que estaba en swap
	 */

	recuperarPaginaSwap(unaPagina,marcoVictima);
}

/*
 * Algoritmo clock modificado que retorna la pagina victima
 * -> Pendientes TODO:
 * 	NO arrancar siempre desde el principio. Debe guardar cual fue la ultima pagina que consulte y empezar a partir de esa
 */

t_pagina * ejecutarAlgoritmoReemplazo()
{
	bool encontrePaginaVictima = true;
	t_pagina * paginaRetorno = NULL;

	void iterarDiccionario(char * proceso,t_list * segmentos)
	{
		void iteradorSegmentos(t_segmento * unSegmento)
		{
			void iteradorPaginas(t_pagina * unaPagina)
			{
				if(unaPagina->uso == 0 && unaPagina->nroPaginaSwap!=-1 && encontrePaginaVictima)
				{
					paginaRetorno = unaPagina;
					encontrePaginaVictima=false;
				}
				else
				{
					unaPagina->uso = 0;
				}

			}
			list_iterate(unSegmento->paginas,(void*)iteradorPaginas);
		}
		list_iterate(segmentos,(void*)iteradorSegmentos);
	}

	dictionary_iterator(diccionarioProcesos,(void*)iterarDiccionario);
	// Si es un buffer circular esto no va a server. No va a arrancar siempre desde la ultima posicion si no que desde el principio
	// Esto complica las cosas => hay que ver como indexamos las paginas y recuperamos este nro!

	if(!paginaRetorno)
	{
		paginaRetorno = ejecutarAlgoritmoReemplazo(); // vuelve a hacer una pasada

		if(!paginaRetorno) // nunca deberia pasar -> caso de error extremo
			return NULL;
	}

	return paginaRetorno;
}

/*
 * Lee la pagina victima en memoria principal y la escribe en memoria swap
 * Luego actualiza su nroPaginaSwap al que corresponda
 */

void reemplazarVictima(t_pagina ** paginaVictima, bool bloqueoMarco)
{
	int marcoVictima = (*paginaVictima)->nroMarco;

	int offsetVictimaMemoriaPrincipal = marcoVictima*tamPagina; // Obtengo el marco asociado a dicha pagina

	int nroPaginaSwapVictima = asignarMarcoLibreSwap(); // Obtengo un marco libre en la memoria swap para dejar al marco de arriba

	void * bufferPagina = leerDeMemoria(offsetVictimaMemoriaPrincipal,tamPagina); // Leo lo que voy a sacar y lo guardo en el buffer (lo de la victima)

	escribirSwap(nroPaginaSwapVictima,bufferPagina); //  Escribo en swap lo de la victima

	(*paginaVictima)->nroPaginaSwap = nroPaginaSwapVictima; // Apunto la pagina swap al nro que obtuve

	if(!bloqueoMarco)
	{
		pthread_mutex_lock(&mutex_algoritmo_reemplazo);
		bitarray_clean_bit(marcosMemoriaPrincipal,marcoVictima);
		pthread_mutex_unlock(&mutex_algoritmo_reemplazo);
	}
}

/*
 * Lee la pagina en memoria swap y la escribe en memoria principal
 * Actualiza su numero de marco (al marco de la victima), su bit de uso y pone su nroPaginaSwap en -1
 */

void recuperarPaginaSwap(t_pagina ** paginaActualmenteEnSwap,int marcoObjetivo)
{
	int nroPaginaSwap = (*paginaActualmenteEnSwap)->nroPaginaSwap; // obtengo el nro de pagina swap a la cual referenciaba

	void * bufferPagina = leerSwap(nroPaginaSwap); // Leo del swap lo que estaba buscando

	escribirEnMemoria(bufferPagina,marcoObjetivo*tamPagina,tamPagina); // Escribo en memoria lo que estaba en el swap

	free(bufferPagina);

	(*paginaActualmenteEnSwap)->nroMarco = marcoObjetivo; // actualizo la referencia al marco
	(*paginaActualmenteEnSwap)->uso = 1; // pongo el uso en 1 ya que recien la traigo
	(*paginaActualmenteEnSwap)->nroPaginaSwap = -1; // Actualizo que no esta mas en archivo swap
}

/*
 * Valida si esta en memoria principal
 */

bool estaEnMemoria(t_list * paginas, int nroPagina)
{
	t_pagina * aux = obtenerPaginaAuxiliar(paginas,nroPagina);

	bool retorno = (aux->nroPaginaSwap == -1);

	free(aux);

	return retorno;
}

void escribirSwap(int nroPagina, void * buffer)
{
	FILE * fd = fopen("Memoria Swap","r+");
	int fd_num = fileno(fd);

	void * bufferAuxiliar = mmap(NULL,tamSwap,PROT_READ|PROT_WRITE,MAP_SHARED,fd_num,0);

	memcpy(bufferAuxiliar + nroPagina*tamPagina,buffer,tamPagina);

	msync(bufferAuxiliar,tamPagina,MS_SYNC);

	munmap(bufferAuxiliar,tamSwap);
}

void * leerSwap(int nroPagina)
{
	void * buffer = malloc(tamPagina);

	FILE * fd = fopen("Memoria Swap","r+");
	int fd_num = fileno(fd);

	void * bufferAuxiliar = mmap(NULL,tamSwap,PROT_READ|PROT_WRITE,MAP_SHARED,fd_num,0);

	memcpy(buffer,bufferAuxiliar + nroPagina*tamPagina,tamPagina);

	msync(bufferAuxiliar,tamPagina,MS_SYNC);

	munmap(bufferAuxiliar,tamSwap);

	return buffer;
}

int asignarMarcoLibreSwap()
{
	for(int i = 0; i < cantidadMarcosMemoriaVirtual; i++) {
			if(estaLibreMarco(i)) {
				pthread_mutex_lock(&mutex_marcos_libres);
				bitarray_set_bit(marcosMemoriaSwap, i);
				pthread_mutex_unlock(&mutex_marcos_libres);
				return i;
			}
		}
	return -1;
}


#endif /* MUSEMEMORIASWAP_H_ */
