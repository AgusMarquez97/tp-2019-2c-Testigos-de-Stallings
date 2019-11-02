/*
 * MuseOperaciones.h
 *
 *  Created on: Oct 31, 2019
 *      Author: agus
 */

#ifndef MUSEOPERACIONES_H_
#define MUSEOPERACIONES_H_

#include "MuseAuxiliares.h"

bool existeEnElDiccionario(char * idProceso)
{
	if(diccionarioProcesos)
	{
		if(!dictionary_is_empty(diccionarioProcesos))
		{
			if(dictionary_has_key(diccionarioProcesos,idProceso))
			{
					return true;
			}
		}
	}
	return false;
}

int procesarHandshake(char * idProceso)
{
	char msj[120];


	if(existeEnElDiccionario(idProceso))
	{
			sprintf(msj,"Fallo al realizar el handshake, init duplicado del proceso %s", idProceso);
			loggearInfo(msj);
			return -1;
	}

	dictionary_put(diccionarioProcesos,idProceso,NULL);
	sprintf(msj,"Handshake exitoso. Proceso %s agregado al diccionario de procesos correctamente",idProceso);
	loggearInfo(msj);

	return 0;
}

uint32_t procesarMalloc(char * idProceso, int32_t tamanio)
{
	char msj[120];
	int cantidadFrames;
	if(existeEnElDiccionario(idProceso))
	{
		cantidadFrames = obtenerCantidadMarcos(tamPagina, tamanio + sizeof(t_heap_metadata)); // Frames necesarios para escribir en memoria
		if(poseeSegmentos(idProceso))
				return evaluarSegmento(idProceso,tamanio,cantidadFrames);
		else
				return crearSegmento(idProceso, tamanio,cantidadFrames);
	}
	else
	{
		sprintf(msj,"Proceso %s no realizo el init correspondiente",idProceso);
		loggearWarning(msj);
		return 0;
	}

}

void escribirPaginas(t_list * listaPaginas, int tamanio)
{
	//t_list * listaHM;

	int offset = 0;


	void escribirPagina(t_pagina * unaPagina)
	{
			/*t_heap_metadata = 5;
			1° pagina[5 bytes del heap {false, 26} - 1 de /0] 26
			2° Pagina[23 bytes de /0 + 5 de heap ]*/

			offset = unaPagina->nroMarco*tamPagina;

			if(unaPagina->nroPagina == 0)
			{
				t_heap_metadata * metadata = malloc(sizeof(t_heap_metadata));

				metadata->estaLibre = false;
				metadata->offset = tamanio;

				//offset += tamanioRestante;

				pthread_mutex_lock(&mutex_memoria);
				memcpy(memoria + offset,metadata,sizeof(t_heap_metadata)); // Escribo la metadata (5 primeros bytes)
				pthread_mutex_unlock(&mutex_memoria);

				offset += sizeof(t_heap_metadata);

				free(metadata);

			}

			if(tamanio + 2*sizeof(t_heap_metadata) >= tamPagina)
			{
				tamanio = tamPagina - sizeof(t_heap_metadata); // Lo que entre en esta pagina 27
			}
			else
			{
				t_heap_metadata * metadataFinal = malloc(sizeof(t_heap_metadata));

				metadataFinal->estaLibre = true;
				metadataFinal->offset = tamPagina - tamanio - sizeof(t_heap_metadata);

				pthread_mutex_lock(&mutex_memoria);
				memcpy(memoria + offset + tamanio,metadataFinal,sizeof(t_heap_metadata)); // Escribo la metadata (5 primeros bytes)
				pthread_mutex_unlock(&mutex_memoria);

				free(metadataFinal);
			}


	}

	list_iterate(listaPaginas,(void *)escribirPagina);
}

t_list * obtenerPaginas(int tamanio, int cantidadFrames)
{
	t_pagina * unaPagina;
	t_list * listaPaginas = list_create();
	for(int aux = 0; aux < cantidadFrames;aux++)
	{
		unaPagina = malloc(sizeof(unaPagina));

		unaPagina->nroMarco = asignarMarcoLibre();
		unaPagina->nroPagina = aux;

		list_add(listaPaginas,unaPagina);
	}
	escribirPaginas(listaPaginas,tamanio); // escribe en MP posta, Memoria ya reservada
	return listaPaginas;
}

t_segmento * instanciarSegmento(int tamanio, int cantidadFrames)
{
	t_list * listaPaginas = obtenerPaginas(tamanio,cantidadFrames);

	t_segmento * primerSegmento = malloc(sizeof(t_segmento));

	primerSegmento->id_segmento = 0;
	primerSegmento->esCompartido = false;
	primerSegmento->posicionInicial = 0;
	primerSegmento->tamanio = cantidadFrames * tamPagina;
	primerSegmento->paginas = listaPaginas;

	return primerSegmento;
}

uint32_t crearSegmento (char * idProceso, int tamanio, int cantidadFrames) // podria ocurrir con mmap?
{
		uint32_t direccionarInicial = sizeof(t_heap_metadata);

		t_list * listaSegmentos = list_create();
		t_segmento * primerSegmento = instanciarSegmento(tamanio,cantidadFrames);

		list_add(listaSegmentos,primerSegmento);

		pthread_mutex_lock(&mutex_diccionario);
		dictionary_remove(diccionarioProcesos, idProceso);
		dictionary_put(diccionarioProcesos,idProceso,listaSegmentos);
		pthread_mutex_unlock(&mutex_diccionario);

		return direccionarInicial;
}

uint32_t evaluarSegmento (char * idProceso, int tamanio, int cantidadFrames)
{

	pthread_mutex_lock(&mutex_diccionario);
	//t_list * segmentodictionary_remove(idProceso)
	pthread_mutex_unlock(&mutex_diccionario);

	return 0;
}


int32_t procesarFree(char * idProceso, uint32_t posicionMemoria) {
		char msj[120];
		int offset;
		int retorno = -1;
		if(existeEnElDiccionario(idProceso))
		{
			t_segmento * segmentoBuscado = obtenerSegmento((t_list*)dictionary_get(diccionarioProcesos,idProceso),posicionMemoria);
			t_pagina * paginaBuscada = obtenerPagina(segmentoBuscado->paginas,posicionMemoria);

			int i = 1;
			offset = posicionMemoria;

			while(tamPagina*i<=posicionMemoria)
			{
				offset = posicionMemoria - tamPagina * i;
				i++;
			}

			retorno = liberarHeapMetadata(paginaBuscada->nroMarco*tamPagina,offset);

			return 0;
		}
		else
		{
			sprintf(msj,"Proceso %s no realizo el init correspondiente",idProceso);
			loggearWarning(msj);
			return retorno;
		}
}

void * procesarGet(char * idProceso, uint32_t posicionMemoria, int32_t tamanio) {
		char msj[120];
		int offset = 0;

		if(existeEnElDiccionario(idProceso))
		{
			t_segmento * segmentoBuscado = obtenerSegmento((t_list*)dictionary_get(diccionarioProcesos,idProceso),posicionMemoria);
			t_pagina * paginaBuscada = obtenerPagina(segmentoBuscado->paginas,posicionMemoria);


			int i = 1;
			offset = posicionMemoria;

			while(tamPagina*i<=posicionMemoria)
			{
				offset = posicionMemoria - tamPagina * i;
				i++;
			}

			t_heap_metadata * unHeapMetadata = obtenerHeapMetadata(paginaBuscada->nroMarco*tamPagina,offset);

			if(unHeapMetadata->estaLibre && tamanio > unHeapMetadata->offset)
				return NULL;

			void * contenidoRetorno = leerDeMemoria(paginaBuscada->nroMarco*tamPagina + offset, tamanio);
			return contenidoRetorno;
		}
		else
		{
			sprintf(msj,"Proceso %s no realizo el init correspondiente",idProceso);
			loggearWarning(msj);
			return NULL;
		}
}

int procesarCpy(char * idProceso, uint32_t posicionMemoria, int32_t tamanio, void* contenido) {

	char msj[120];
	int offset = 0;
	int retorno = -1;

	if(existeEnElDiccionario(idProceso))
	{
		t_segmento * segmentoBuscado = obtenerSegmento((t_list*)dictionary_get(diccionarioProcesos,idProceso),posicionMemoria);
		t_pagina * paginaBuscada = obtenerPagina(segmentoBuscado->paginas,posicionMemoria);


		int i = 1;
		offset = posicionMemoria;

		while(tamPagina*i<=posicionMemoria)
		{
			offset = posicionMemoria - tamPagina * i;
			i++;
		}

		t_heap_metadata * unHeapMetadata = obtenerHeapMetadata(paginaBuscada->nroMarco*tamPagina,offset);

		if(unHeapMetadata->estaLibre && tamanio > unHeapMetadata->offset)
		{
			return -1;
		}
		copiarEnMemoria(contenido,paginaBuscada->nroMarco*tamPagina + offset, tamanio);
		return 0;
	}
	else
	{
		sprintf(msj,"Proceso %s no realizo el init correspondiente",idProceso);
		loggearWarning(msj);
		return retorno;
	}

}

uint32_t procesarMap(char * idProceso, void* contenido, int32_t tamanio, int32_t flag) {

	char* pathArchivo = malloc(tamanio + 1);

	memcpy(pathArchivo, contenido, tamanio);
	pathArchivo[tamanio] = 0;
	loggearInfo(pathArchivo);
	free(pathArchivo);

	return 1;

}

int procesarSync(char * idProceso, uint32_t posicionMemoria, int32_t tamanio) {
	return 0;
}

int procesarUnmap(char * idProceso, uint32_t posicionMemoria) {
	return 0;
}

int procesarClose(char * idProceso) {

	char msj[120];

	t_list * segmentos;

	segmentos = dictionary_get(diccionarioProcesos,idProceso);

	if(segmentos != NULL)
	{
		if(!list_is_empty(segmentos))
		{
			void liberarSegmento(t_segmento * unSegmento)
			{
				// Liberar Segmento....
			}

			list_iterate(segmentos,(void*)liberarSegmento);
			list_destroy(segmentos);
		}
	}

	dictionary_remove(diccionarioProcesos,idProceso);

	sprintf(msj,"Proceso %s eliminado del diccionario de procesos",idProceso);
	loggearInfo(msj);

	return 0;
}






#endif /* MUSEOPERACIONES_H_ */
