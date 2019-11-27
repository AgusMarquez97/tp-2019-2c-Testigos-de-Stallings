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
		cantidadFrames = obtenerCantidadMarcos(tamPagina, tamanio + tam_heap_metadata); // Frames necesarios para escribir en memoria
		if(poseeSegmentos(idProceso))
				return analizarSegmento(idProceso,tamanio,cantidadFrames,false);
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

void escribirPaginas(int cantidadPaginas, int tamanio, int primerMarco, int ultimoMarco)
{
    int tamanioTotalReservado = cantidadPaginas*tamPagina;
    int tamanioEscribir = tam_heap_metadata + tamanio; // Lo que se escribe obligatoriamente en memoria
    int tamanioSobrante = tamanioTotalReservado - tamanioEscribir; // Los bytes que me sobran o que no uso


	int offset = primerMarco*tamPagina; // Primer marco donde voy a escribir la heap metadata, voy a estar parado al inicio de la primer pagina que me den

    t_heap_metadata * metadata = malloc(tam_heap_metadata);

    metadata->estaLibre = false;
    metadata->offset = tamanio;

    char aux[300];

    sprintf(aux,"Se carga en memoria un t_heap_metadata con offset de %d bytes en el marco %d",metadata->offset,primerMarco);
    loggearInfo(aux);


    pthread_mutex_lock(&mutex_memoria);
    memcpy(memoria + offset,metadata,tam_heap_metadata); // Escribo la metadata (5 primeros bytes)
    pthread_mutex_unlock(&mutex_memoria);

    free(metadata);

    if(tamanioSobrante > tam_heap_metadata) // quiere decir que me sobra suficiente lugar en la ultima pagina para escribir un metadata libre = true
    {

        offset = ultimoMarco*tamPagina + (tamPagina - tamanioSobrante); // ver si hay que restar 1
        t_heap_metadata * metadata = malloc(tam_heap_metadata);

        metadata->estaLibre = true;
        metadata->offset = tamanioSobrante - tam_heap_metadata;


        sprintf(aux,"Se carga en memoria un segundo t_heap_metadata con offset de %d bytes en el marco %d",metadata->offset,ultimoMarco);
        loggearInfo(aux);

        pthread_mutex_lock(&mutex_memoria);
        memcpy(memoria + offset,metadata,tam_heap_metadata); // Escribo la metadata (5 primeros bytes)
        pthread_mutex_unlock(&mutex_memoria);

        free(metadata);
    }
    else
    {
    	loggearInfo("Fragmentacion interna...");
    }
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
	int cantidadMarcos = list_size(listaPaginas);
	int primerMarco = ((t_pagina*)list_get(listaPaginas,0))->nroMarco;
	int ultimoMarco = ((t_pagina*)list_get(listaPaginas,cantidadMarcos-1))->nroMarco;

	escribirPaginas(cantidadMarcos, tamanio, primerMarco, ultimoMarco); // escribe en MP posta, Memoria ya reservada
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
		uint32_t direccionarInicial = tam_heap_metadata;

		t_list * listaSegmentos = list_create();
		t_segmento * primerSegmento = instanciarSegmento(tamanio,cantidadFrames);

		list_add(listaSegmentos,primerSegmento);

		pthread_mutex_lock(&mutex_diccionario);
		dictionary_remove(diccionarioProcesos, idProceso);
		dictionary_put(diccionarioProcesos,idProceso,listaSegmentos);
		pthread_mutex_unlock(&mutex_diccionario);

		return direccionarInicial;
}

void crearSegmento2 (char * idProceso, int tamanio, int cantidadFrames,t_list * listaSegmentos, int idSegmento, bool esCompartido, int posicionInicial) // podria ocurrir con mmap?
{
		t_segmento * unSegmento = instanciarSegmento2(tamanio,cantidadFrames, idSegmento, esCompartido, posicionInicial);

		list_add(listaSegmentos,unSegmento);

		pthread_mutex_lock(&mutex_diccionario);
		dictionary_remove(diccionarioProcesos, idProceso);
		dictionary_put(diccionarioProcesos,idProceso,listaSegmentos);
		pthread_mutex_unlock(&mutex_diccionario);
}

t_segmento * instanciarSegmento2(int tamanio, int cantidadFrames, int idSegmento, bool esCompartido, int posicionInicial)
{
	t_list * listaPaginas = obtenerPaginas2(tamanio,cantidadFrames);

	t_segmento * unSegmento = malloc(sizeof(t_segmento));

	unSegmento->id_segmento = idSegmento;
	unSegmento->esCompartido = esCompartido;
	unSegmento->posicionInicial = posicionInicial;
	unSegmento->tamanio = cantidadFrames * tamPagina;
	unSegmento->paginas = listaPaginas;

	return unSegmento;
}

t_list * obtenerPaginas2(int tamanio, int cantidadFrames)
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

	int primerFrame = ((t_pagina*)list_get(listaPaginas,0))->nroMarco;
	int ultimoFrame = ((t_pagina*)list_get(listaPaginas,cantidadFrames-1))->nroMarco;

	escribirPaginas(cantidadFrames, tamanio, primerFrame, ultimoFrame); // escribe en MP posta, Memoria ya reservada
	return listaPaginas;
}

void escribirPaginas(int cantidadPaginas, int tamanio, int primerMarco, int ultimoMarco)
{
    int tamanioTotalReservado = cantidadPaginas*tamPagina;
    int tamanioEscribir = tam_heap_metadata + tamanio; // Lo que se escribe obligatoriamente en memoria
    int tamanioSobrante = tamanioTotalReservado - tamanioEscribir; // Los bytes que me sobran o que no uso


	int offset = primerMarco*tamPagina; // Primer marco donde voy a escribir la heap metadata, voy a estar parado al inicio de la primer pagina que me den

    t_heap_metadata * metadata = malloc(tam_heap_metadata);

    metadata->estaLibre = false;
    metadata->offset = tamanio;

    char aux[300];

    sprintf(aux,"Se carga en memoria un t_heap_metadata con offset de %d bytes en el marco %d",metadata->offset,primerMarco);
    loggearInfo(aux);


    pthread_mutex_lock(&mutex_memoria);
    memcpy(memoria + offset,metadata,tam_heap_metadata); // Escribo la metadata (5 primeros bytes)
    pthread_mutex_unlock(&mutex_memoria);

    free(metadata);

    if(tamanioSobrante > tam_heap_metadata) // quiere decir que me sobra suficiente lugar en la ultima pagina para escribir un metadata libre = true
    {

        offset = ultimoMarco*tamPagina + (tamPagina - tamanioSobrante); // ver si hay que restar 1
        t_heap_metadata * metadata = malloc(tam_heap_metadata);

        metadata->estaLibre = true;
        metadata->offset = tamanioSobrante - tam_heap_metadata;


        sprintf(aux,"Se carga en memoria un segundo t_heap_metadata con offset de %d bytes en el marco %d",metadata->offset,ultimoMarco);
        loggearInfo(aux);

        pthread_mutex_lock(&mutex_memoria);
        memcpy(memoria + offset,metadata,tam_heap_metadata); // Escribo la metadata (5 primeros bytes)
        pthread_mutex_unlock(&mutex_memoria);

        free(metadata);
    }
    else
    {
    	loggearInfo("Fragmentacion interna...");
    }
}

 //Que pasa si en el medio de esta operacion el mismo proceso mediante otro hilo me inserta otro segmento al mismo tiempo = PROBLEMAS
uint32_t analizarSegmento (char * idProceso, int tamanio, int cantidadFrames, bool esCompartido)
{
	/*
	 * Por ahora no evaluo el caso de que haya memoria virtual => necesite crear un nuevo segmento
	 */

	if(poseeSegmentos(idProceso))
	{
		t_list * lista_segmentos = list_create();
		crearSegmento2 (idProceso,tamanio,cantidadFrames,lista_segmentos,0,false,0);
		return tam_heap_metadata; //Es el primer segmento que se crea...
	}

	uint32_t direccionRetorno;
	t_list * listaSegmentos;

	t_segmento * ultimoSegmento;

	pthread_mutex_lock(&mutex_diccionario);
	listaSegmentos = dictionary_get(diccionarioProcesos,idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	ultimoSegmento = list_get(listaSegmentos,list_size(listaSegmentos)-1);

	if(ultimoSegmento->esCompartido == true && esCompartido == false)
	{
		// Creo un nuevo segmento luego del compartido...

		t_segmento * segmentoNuevo = crearSegmentoNuevo(tamanio,ultimoSegmento->id_segmento+1);

		direccionRetorno = ultimoSegmento->posicionInicial + ultimoSegmento->tamanio + tam_heap_metadata;
	}
	else
	{
		// Agrando el ultimo segmento
		int tamanioSobranteUltimoMarco = calcularSobrante(ultimoSegmento);
		int nuevaCantidadFrames = obtenerCantidadMarcos(tamPagina, tamanio + tam_heap_metadata - tamanioSobranteUltimoMarco); // Frames necesarios para escribir en memoria
		agrandarSegmento();
	}

	return direccionRetorno;
}

int calcularSobrante(t_segmento * unSegmento)
{
	t_list * paginas;
	t_pagina * unaPagina;
	t_pagina * ultimaPagina;

	t_pagina * unaPagina;

	int offset = 0;
	int tamanioAvanzar = 0;
	int tamanioLeido = 0;
	int cantidadPaginas = 0;

	t_heap_metadata * unHeapMetadata = malloc(sizeof(t_heap_metadata));

	offset =


	pthread_mutex_lock(&mutex_memoria);
	memcpy(unHeapMetadata,memoria + offset,tamanioPartido); // Escribo la metadata (5 primeros bytes)
	pthread_mutex_unlock(&mutex_memoria);




	int tamanioMaximo = list_size(paginas)*tamPagina;

	unaPagina = list_get(paginas,0);


		while(tamanioLeido < tamPagina)
		{
		offset = unaPagina->nroMarco * tamPagina;

		if(tamanioLeido > tamanioMaximo)
		{
			break;
		}

		void * buffer = malloc(tam_heap_metadata);

		memcpy(unHeapMetadata,buffer,tam_heap_metadata);



		memcpy(buffer,unHeapMetadata,tam_heap_metadata);

		t_heap_metadata * unHeapMetadata = malloc(5)

		unHeapMetadata = (t_heap_metadata *) buffer;


		tamanioLeido += unHeapMetadata->offset + tam_heap_metadata;

		offset += tam_heap_metadata;

		tamanioAvanzar = unHeapMetadata->offset;

				if(tamanioAvanzar > tamPagina - tam_heap_metadata)
				{

				}
		}



	}







}




int calcularSobrante(t_segmento * unSegmento)
{
	int sobrante = 0;

	t_pagina * unaPagina;

	int offset = 0;
	int bytesLeidos = 0;

	int cantidadPaginas = 0;
	int tamanioMaximo = 0;
	void * buffer = malloc(tam_heap_metadata);

	t_list * paginas = unSegmento->paginas;

	t_pagina * primerPagina = list_get(paginas,0); // PAGINA 0
	cantidadPaginas = list_size(paginas); // 2

	tamanioMaximo = tamPagina * cantidadPaginas; //64

	t_heap_metadata * unHeapMetadata = malloc(sizeof(t_heap_metadata));

	offset = primerPagina->nroMarco*tamPagina;//0

	pthread_mutex_lock(&mutex_memoria);
	memcpy(unHeapMetadata,memoria + offset,tam_heap_metadata); // Escribo la metadata (5 primeros bytes)
	pthread_mutex_unlock(&mutex_memoria);

	bytesLeidos += tam_heap_metadata; //5

	if(unHeapMetadata->offset + tam_heap_metadata < tamPagina) //
	{
		bytesLeidos += unHeapMetadata->offset; //29
	}

	sobrante = tamanioMaximo - bytesLeidos; //64-29 = 35
	int i = 0;
	while(sobrante > tam_heap_metadata || !unHeapMetadata->estaLibre) // sabemos que hay un heap mas
	{
		offset = primerPagina->nroMarco*tamPagina + bytesLeidos;

		//leer un heapMetadata

		if((tamPagina - bytesLeidos)>tam_heap_metadata)
		{

		}

		else
		{

		pthread_mutex_lock(&mutex_memoria);
		memcpy(buffer,memoria + offset,(tamPagina - bytesLeidos)); // Escribo la metadata (3 primeros bytes)
		pthread_mutex_unlock(&mutex_memoria);

		i++;

		unaPagina = list_get(paginas,i);

		offset = unaPagina->nroMarco*tamPagina;

		pthread_mutex_lock(&mutex_memoria);
		memcpy(buffer,memoria + offset,tam_heap_metadata - (tamPagina - bytesLeidos)); // Escribo la metadata (3 primeros bytes)
		pthread_mutex_unlock(&mutex_memoria);

		memcpy(unHeapMetadata,buffer,tam_heap_metadata);

		bytesLeidos += (tam_heap_metadata + unHeapMetadata->offset); // 34 + 17 = 51

		// leo el heap y llego a que es libre

		}

		sobrante = tamanioMaximo - bytesLeidos; //64-51 = 13
	}


		if(sobrante < tam_heap_metadata)
			sobrante = unHeapMetadata->offset + tam_heap_metadata;

		free(unHeapMetadata);
		free(buffer);

		return sobrante;

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

			return 1;
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
					void liberarPaginas(t_pagina * unaPagina)
					{
						liberarMarcoBitarray(unaPagina->nroMarco);
						free(unaPagina);
					}

					list_iterate(unSegmento->paginas,(void*)liberarPaginas);
					list_destroy(unSegmento->paginas);

					free(unSegmento);
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
