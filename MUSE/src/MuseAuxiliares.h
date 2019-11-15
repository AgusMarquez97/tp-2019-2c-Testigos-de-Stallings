/*
 * MuseAuxiliares.h
 *
 *  Created on: Oct 31, 2019
 *      Author: agus
 */

#ifndef MUSEAUXILIARES_H_
#define MUSEAUXILIARES_H_

#include "MUSE.h"


// Auxiliares:
void liberarMarcoBitarray(int nroMarco)
{
	pthread_mutex_lock(&mutex_marcos_libres);
	bitarray_clean_bit(marcosMemoriaPrincipal,nroMarco);
	pthread_mutex_unlock(&mutex_marcos_libres);
}

int asignarMarcoLibre()
{
	for(int i = 0;i < cantidadMarcosMemoriaPrincipal; i++)
	{
		if(marcoLibreMP(i))
		{
			pthread_mutex_lock(&mutex_marcos_libres);
			bitarray_set_bit(marcosMemoriaPrincipal,i);
			pthread_mutex_unlock(&mutex_marcos_libres);
			return i;
		}
	}
	return -1;
}

bool marcoLibreMP(int nroMarco)
{
	return bitarray_test_bit(marcosMemoriaPrincipal,nroMarco)==0;
}

int obtenerCantidadMarcos(int tamanioPagina, int tamanioMemoria)
{
	float tam_nec = (float)tamanioMemoria/tamanioPagina;
	if(tam_nec == (int)tam_nec)
	return (int)tam_nec;
	return (int)round(tam_nec + 0.5);
}

bool poseeSegmentos(char * idProceso)
{
	return (dictionary_get(diccionarioProcesos,idProceso) != NULL);
}

t_segmento * obtenerSegmento(t_list * segmentos, uint32_t posicionMemoria)
{
	bool segmentoCorrespondiente(t_segmento * unSegmento)
	{
		return (posicionMemoria >= unSegmento->posicionInicial && posicionMemoria <= (unSegmento->posicionInicial+1)*unSegmento->tamanio);
	}
	return list_find(segmentos,(void*)segmentoCorrespondiente);
}

t_pagina * obtenerPagina(t_list * paginas, uint32_t posicionMemoria)
{
	bool paginaCorrespondiente(t_pagina * unaPagina)
	{
		return (posicionMemoria >= unaPagina->nroPagina && posicionMemoria <= (unaPagina->nroPagina+1)*tamPagina);
	}
	return list_find(paginas,(void*)paginaCorrespondiente);
}

t_heap_metadata * obtenerHeapMetadata(int base,int offset)
{
	t_heap_metadata * unHeapMetadata = malloc(sizeof(t_heap_metadata));

	pthread_mutex_lock(&mutex_memoria);
	memcpy(unHeapMetadata,memoria + base + offset - sizeof(t_heap_metadata),sizeof(t_heap_metadata));
	pthread_mutex_unlock(&mutex_memoria);

	return unHeapMetadata;
}

int liberarHeapMetadata(int base,int offset)
{
	t_heap_metadata * unHeapMetadata = obtenerHeapMetadata(base,offset);

	if(!unHeapMetadata)
	{
		return -1;
	}

	unHeapMetadata->estaLibre = true;

	pthread_mutex_lock(&mutex_memoria);
	memcpy(memoria + base + offset - sizeof(t_heap_metadata),unHeapMetadata,sizeof(t_heap_metadata));
	pthread_mutex_unlock(&mutex_memoria);

	free(unHeapMetadata);

	return 0;
}

void copiarEnMemoria(void * contenido, int posicionInicial, int tamanio)
{
	pthread_mutex_lock(&mutex_memoria);
	memcpy(memoria + posicionInicial,contenido,tamanio);
	pthread_mutex_unlock(&mutex_memoria);
}


void * leerDeMemoria(int posicionInicial, int tamanio)
{
	void * buffer = malloc(tamanio);
	pthread_mutex_lock(&mutex_memoria);
	memcpy(buffer,memoria + posicionInicial,tamanio);
	pthread_mutex_unlock(&mutex_memoria);
	return buffer;
}

/*
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





#endif /* MUSEAUXILIARES_H_ */
