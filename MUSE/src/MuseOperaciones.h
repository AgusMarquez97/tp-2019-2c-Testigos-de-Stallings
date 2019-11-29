/*
 * MuseOperaciones.h
 *
 *  Created on: Oct 31, 2019
 *      Author: agus
 */

/*
 * Objetivo de hoy; limpiar el codigo!!
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
		return analizarSegmento(idProceso,tamanio,cantidadFrames,false);
	}
	else
	{
		sprintf(msj,"Proceso %s no realizo el init correspondiente",idProceso);
		loggearWarning(msj);
		return 0;
	}

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

void crearSegmento(char * idProceso, int tamanio, int cantidadFrames,t_list * listaSegmentos, int idSegmento, bool esCompartido, int posicionInicial) // podria ocurrir con mmap?
{
		t_segmento * unSegmento = instanciarSegmento(tamanio,cantidadFrames, idSegmento, esCompartido, posicionInicial);

		list_add(listaSegmentos,unSegmento);

		pthread_mutex_lock(&mutex_diccionario);
		dictionary_remove(diccionarioProcesos, idProceso);
		dictionary_put(diccionarioProcesos,idProceso,listaSegmentos);
		pthread_mutex_unlock(&mutex_diccionario);
}

t_segmento * instanciarSegmento(int tamanio, int cantidadFrames, int idSegmento, bool esCompartido, int posicionInicial)
{
	t_list * listaPaginas = obtenerPaginas(tamanio,cantidadFrames);

	t_segmento * unSegmento = malloc(sizeof(t_segmento));

	unSegmento->id_segmento = idSegmento;
	unSegmento->esCompartido = esCompartido;
	unSegmento->posicionInicial = posicionInicial;
	unSegmento->tamanio = cantidadFrames * tamPagina;
	unSegmento->paginas = listaPaginas;

	return unSegmento;
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

	int primerFrame = ((t_pagina*)list_get(listaPaginas,0))->nroMarco;
	int ultimoFrame = ((t_pagina*)list_get(listaPaginas,cantidadFrames-1))->nroMarco;

	escribirPaginas(cantidadFrames, tamanio, primerFrame, ultimoFrame); // escribe en MP posta, Memoria ya reservada
	return listaPaginas;
}
 //Que pasa si en el medio de esta operacion el mismo proceso mediante otro hilo me inserta otro segmento al mismo tiempo = PROBLEMAS
uint32_t analizarSegmento (char * idProceso, int tamanio, int cantidadFrames, bool esCompartido)
{
	if(poseeSegmentos(idProceso))
	{
		//t_list * lista_segmentos = list_create();
		//crearSegmento(idProceso,tamanio,cantidadFrames,lista_segmentos,0,esCompartido,0); // HACER
		return tam_heap_metadata;
	}

	uint32_t direccionRetorno;
	t_list * listaSegmentos;

	t_segmento * ultimoSegmento;

	pthread_mutex_lock(&mutex_diccionario);
	listaSegmentos = dictionary_get(diccionarioProcesos,idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	ultimoSegmento = list_get(listaSegmentos,list_size(listaSegmentos)-1);

	if(ultimoSegmento->esCompartido)
	{
		// Creo un nuevo segmento luego del compartido...

		//t_segmento * segmentoNuevo = crearSegmentoNuevo(tamanio,ultimoSegmento->id_segmento+1); // HACER

		direccionRetorno = ultimoSegmento->posicionInicial + ultimoSegmento->tamanio + tam_heap_metadata;
	}
	else
	{
		direccionRetorno = completarSegmento(idProceso,ultimoSegmento,tamanio);
	}

	return direccionRetorno;
}


uint32_t completarSegmento(char * idProceso,t_segmento* segmento, int tamanio) {

	uint32_t posicionRetorno = 0;

	int cantPaginas = list_size(segmento->paginas);
	int tamMaximo = tamPagina * cantPaginas;
	t_pagina* unaPagina = list_get(segmento->paginas, 0);
	int offset = unaPagina->nroMarco * tamPagina;
	int bytesLeidos = 0;
	t_heap_metadata* heapMetadata = malloc(sizeof(t_heap_metadata)); // liberar
	bool ultimaPagina = false;

	int sobrante = 0;

	if(cantPaginas==1)
	ultimaPagina = true;

	//auxliares
	int contador = 0;
	int bytesLeidosPagina = 0;

	leerHeapMetadata(&heapMetadata,&bytesLeidos,&bytesLeidosPagina, &offset,&segmento,&contador);

	while((!heapMetadata->estaLibre && !ultimaPagina) || (tamMaximo - bytesLeidos) > tam_heap_metadata)
	{
				if(heapMetadata->estaLibre && heapMetadata->offset <= tamanio)
				{
					escribirHeapMetadata(&heapMetadata,&bytesLeidos,&bytesLeidosPagina,&segmento,&offset,&contador,&posicionRetorno);
					return posicionRetorno;
				}

				leerHeapMetadata(&heapMetadata,&bytesLeidos,&bytesLeidosPagina, &offset,&segmento,&contador);

				if(contador == cantPaginas) // REVISAR OPERACION
					ultimaPagina = true;
	}


	if(heapMetadata->estaLibre)
	sobrante = heapMetadata->offset + tam_heap_metadata; // me sobro un hm entero
	else
	sobrante = (tamMaximo - bytesLeidos); // me sobran n bytes < tam_hm

	int nuevaCantidadFrames = obtenerCantidadMarcos(tamPagina, tamanio + tam_heap_metadata - sobrante); // Frames necesarios para escribir en memoria

	posicionRetorno =  estirarSegmento(idProceso,segmento, tamanio,nuevaCantidadFrames,offset,sobrante);

	return posicionRetorno;// direccion de retorno

	// SI SALGO DEL WHILE SE DOS COSAS:
				// CASO 1: TENGO EL HM METADATA LIBRE CON TANTOS BYTES Y PUEDO USARLO O AGRANDARLO
							//OPCION 1: USAR ULTIMO HM FINALMENTE RETORNAR PRIMERA DIRECCION LUEGO DEL HM QUE ESTABA FREE
							//OPCION 2:
							/*
							ENTONCES:
							AGRANDAR, OSEA PEDIR MARCOS (PEDIR MARCOS NECESARIA=OS EN BASE AL NUEVO TAMANIO NECESARIO) Y USAR LO QUE TENIA + LO NUEVO
							TAM_NECESARIO = TAMANIO - TAM_HM - TAM_OFFSET_HM
							CANTIDAD_FRAMES_NECESARIOS = obtenerCantidadMarcos(tamPagina,TAM_NECESARIO); // Frames necesarios para escribir en memoria
							FINALMENTE: CREAR N PAGINAS NUEVAS CON N MARCOS PEDIDOS + AGRANDAR EL SEGMENTO=N*TAMPAG + RETORNAR PRIMER BYTE LUEGO DEL HM QUE ESTABA FREE
							 */

		// CASO 2: ME SOBRARON N BYTES < TAM HM
							//PASO 1; PEDIR MARCOS NECESARIOS EN BASE AL NUEVO TAMANIO NECESARIO Y ESCRIBIR EL HM PARTIDO (USAR BUFFER)
							// TAM_NECESARIO = TAMANIO - tamMaximo - bytesLeidos
							/*
							 *
							TAM_NECESARIO = TAMANIO - TAM_SOBRANTE
							CANTIDAD_FRAMES_NECESARIOS = obtenerCantidadMarcos(tamPagina,TAM_NECESARIO); // Frames necesarios para escribir en memoria
							FINALMENTE: CREAR N PAGINAS NUEVAS CON N MARCOS PEDIDOS + AGRANDAR EL SEGMENTO=N*TAMPAG + RETORNAR PRIMER BYTE DEL PRIMER HM NUEVO
							 */
}


uint32_t estirarSegmento(char * idProceso,t_segmento * segmento,int tamanio,int nuevaCantidadFrames,int offset, int sobrante)
{

	for(int i = 0;i<nuevaCantidadFrames;i++)
	{
		;
		//crear paginas nuevas 1 por 1
		//Agregar las paginas a la lista de paginas del segmento
	}
	//Actualizar el tamanio del segmento
	//Actualizar el/los hm correspondiente/s (max = 2, min =1)

	return 0;
}

/*
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
*/



#endif /* MUSEOPERACIONES_H_ */
