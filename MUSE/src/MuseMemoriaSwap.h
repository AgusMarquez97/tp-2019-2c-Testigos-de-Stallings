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

	loggearInfo("Se expulsa una pagina de memoria principal a swap");

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
		loggearInfo("Page fault generado");

		t_pagina * paginaVictima = ejecutarAlgoritmoReemplazo(); // obtengo la pagina que quiero reemplazar

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
		int cantidadIntentos = 0;
		int contadorPaginas = 0;
		t_pagina * paginaVictima = NULL;


		pthread_mutex_lock(&mutex_lista_paginas);

		bool estaEnMemoriaLocal(t_pagina * unaPagina)
		{
			return (unaPagina->nroPaginaSwap==-1);
		}

		t_list * lista_analizar = list_filter(listaPaginasClockModificado,(void*)estaEnMemoriaLocal);

		while(cantidadIntentos != 3 || !paginaVictima)
			{
				while(contadorPaginas<list_size(lista_analizar))
				{
					t_pagina * paginaAnalizada = list_get(lista_analizar,ptrAlgoritmoPaginaSiguiente);

					if(paginaAnalizada->uso == false && paginaAnalizada->modificada == false)
					{
						paginaVictima = paginaAnalizada; // caso feliz
						break;
					}
					else if(paginaAnalizada->uso == false && paginaAnalizada->modificada == true && cantidadIntentos>=1)
					{
						paginaVictima = paginaAnalizada;
						break;
					}
					else if(cantidadIntentos>=1)
					{
						paginaAnalizada->uso=false;
					}

					ptrAlgoritmoPaginaSiguiente++;
					contadorPaginas++;

					if(ptrAlgoritmoPaginaSiguiente==list_size(lista_analizar))
						ptrAlgoritmoPaginaSiguiente=0;// da la vuelta
				}
				contadorPaginas=0;
				cantidadIntentos++;
			}

		pthread_mutex_unlock(&mutex_lista_paginas);

		list_destroy(lista_analizar);

	return paginaVictima;
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

	free(bufferPagina);

	(*paginaVictima)->nroPaginaSwap = nroPaginaSwapVictima; // Apunto la pagina swap al nro que obtuve

	if(!bloqueoMarco)
	{
		liberarMarcoBitarray(marcoVictima);
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
	(*paginaActualmenteEnSwap)->modificada = 0; // pongo el uso en 1 ya que recien la traigo
	(*paginaActualmenteEnSwap)->nroPaginaSwap = -1; // Actualizo que no esta mas en archivo swap

	if(nroPaginaSwap!=-2)
	liberarPaginasSwap(nroPaginaSwap);
}

void escribirSwap(int nroPagina, void * buffer)
{
	FILE * fd = fopen("Memoria Swap","r+");
	int fd_num = fileno(fd);

	void * bufferAuxiliar = mmap(NULL,tamSwap,PROT_READ|PROT_WRITE,MAP_SHARED,fd_num,0);

	memcpy(bufferAuxiliar + nroPagina*tamPagina,buffer,tamPagina);

	//msync(bufferAuxiliar,tamPagina,MS_SYNC);

	munmap(bufferAuxiliar,tamSwap);

	close(fd_num);
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

	close(fd_num);

	return buffer;
}

/*
 * Valida si esta en memoria principal
 */

bool estaEnMemoria(t_list * paginas, int nroPagina)
{
	t_pagina * aux = obtenerPaginaAuxiliar(paginas,nroPagina);

	bool retorno = (aux->nroPaginaSwap == -1 || aux->nroPaginaSwap == -2 );

	free(aux);

	return retorno;
}

int asignarMarcoLibreSwap()
{
	for(int i = 0; i < cantidadMarcosMemoriaVirtual; i++) {
			if(estaLibreMarcoMemoriaSwap(i)) {
				pthread_mutex_lock(&mutex_marcos_libres);
				bitarray_set_bit(marcosMemoriaSwap, i);
				pthread_mutex_unlock(&mutex_marcos_libres);
				return i;
			}
		}
	return -1;
}

bool estaLibreMarcoMemoriaSwap(int nroMarco) {
	pthread_mutex_lock(&mutex_marcos_swap_libres);
	bool retorno = bitarray_test_bit(marcosMemoriaSwap, nroMarco) == 0;
	pthread_mutex_unlock(&mutex_marcos_swap_libres);
	return retorno;

}

void* leerDeMemoria(int posicionInicial, int tamanio)
{

	void* buffer = malloc(tamanio);
	memcpy(buffer, memoria + posicionInicial, tamanio); // no hacen falta los locks, ya estan puestos antes

	return buffer;

}

void escribirEnMemoria(void* contenido, int posicionInicial, int tamanio)
{
	memcpy(memoria + posicionInicial, contenido, tamanio);
}

void eliminarDeAlgoritmo(t_pagina * unaPagina)
{
	bool existeEnAlgoritmo(t_pagina * unaPaginaLocal)
	{
		return unaPaginaLocal == unaPagina; // validar
	}

	pthread_mutex_lock(&mutex_algoritmo_reemplazo);
	list_remove_by_condition(listaPaginasClockModificado,(void*)existeEnAlgoritmo);
	pthread_mutex_unlock(&mutex_algoritmo_reemplazo);
}

#endif /* MUSEMEMORIASWAP_H_ */
