#ifndef MUSEOPERACIONES_H_
#define MUSEOPERACIONES_H_

#include "MuseHeapMetadata.h"

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

int procesarHandshake(char* idProceso) {

	char msj[120];

	if(existeEnElDiccionario(idProceso)) {
		sprintf(msj, "Fallo al realizar el handshake, init duplicado del proceso %s", idProceso);
		loggearError(msj);
		return -1;
	}

	pthread_mutex_lock(&mutex_diccionario);
	dictionary_put(diccionarioProcesos, idProceso, NULL);
	pthread_mutex_unlock(&mutex_diccionario);


	sprintf(msj, "Handshake exitoso. Proceso %s agregado al diccionario de procesos correctamente", idProceso);
	loggearInfo(msj);

	return 0;

}

uint32_t procesarMalloc(char* idProceso, int32_t tamanio) {

	char msj[200];
	int cantidadFrames;
	uint32_t posicionRetorno = 0;

	if(existeEnElDiccionario(idProceso)) {
		cantidadFrames = obtenerCantidadMarcos(tamPagina, tamanio + tam_heap_metadata);
		posicionRetorno = analizarSegmento(idProceso, tamanio, cantidadFrames,false);
		if(posicionRetorno == 0)
			sprintf(msj,"Error en el malloc del proceso %s: memoria llena",idProceso);
		else
			sprintf(msj,"Malloc para el proceso %s retorna la posicion %u",idProceso,posicionRetorno);

		loggearInfo(msj);
	} else	{
		sprintf(msj, "El proceso %s no realizo el init correspondiente", idProceso);
		loggearWarning(msj);
	}

	return posicionRetorno;

}

int defragmentarSegmento(t_segmento* segmento)
{
	t_list* listaPaginas = segmento->paginas;

	t_pagina* paginaAuxiliar = obtenerPaginaAuxiliar(listaPaginas, 0);

	int offset = paginaAuxiliar->nroMarco * tamPagina;

	free(paginaAuxiliar);

	int cantPaginas = list_size(listaPaginas);
	int tamMaximo = tamPagina * cantPaginas;
	int bytesLeidos = 0;
	t_heap_metadata* heapMetadata = malloc(tam_heap_metadata);

	int bytesLeidosPagina = 0;
	int contador = 0;
	int heapsLeidos = 0;
	int primerHeapMetadataLibre = 0;
	int primeraPaginaLibre = 0;
	int acumulador = 0;
	int cantidadBytesAgrupados = 0;
	int paginaUltimoHeapMetadata = 0;

	int offsetAnterior = 0;

	while(tamMaximo - bytesLeidos > tam_heap_metadata)
	{
		offsetAnterior = offset;
		paginaUltimoHeapMetadata = contador;
		leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, listaPaginas, &contador);

		if(heapMetadata->estaLibre)
		{
			heapsLeidos++;

			if(heapsLeidos == 1)
			{
				primeraPaginaLibre = paginaUltimoHeapMetadata;
				primerHeapMetadataLibre = offsetAnterior;
			}


			acumulador += heapMetadata->offset;

			if(heapsLeidos > 1)
			{
				heapMetadata->estaLibre = true;
				heapMetadata->offset = acumulador + tam_heap_metadata;

				int tamanioRestante = tamPagina - primerHeapMetadataLibre%tamPagina;

				escribirUnHeapMetadata(listaPaginas, primeraPaginaLibre, heapMetadata, &primerHeapMetadataLibre, tamanioRestante);
				cantidadBytesAgrupados = heapMetadata->offset;

				break;
			}
		}
		else
		{
			heapsLeidos = 0;
			acumulador = 0;
		}
	}
	free(heapMetadata);

	return cantidadBytesAgrupados;

}

void compactarSegmento(char* idProceso, t_segmento* segmento) {

	t_list* listaPaginas = segmento->paginas;

	t_pagina* paginaAuxiliar = obtenerPaginaAuxiliar(listaPaginas, 0);

	int offset = paginaAuxiliar->nroMarco * tamPagina;

	free(paginaAuxiliar);

	t_heap_metadata* heapMetadata = malloc(tam_heap_metadata);
	int bytesLeidos = 0;
	int bytesLeidosPagina = 0;
	t_list* paginas = segmento->paginas;
	int cantPaginas = list_size(paginas);
	int tamMaximo = tamPagina * cantPaginas;
	int nroPagina = 0;

	int posUltimoHeapMetadata = 0;
	int paginaUltimoHeapMetadata = 0;
	int tamanioPaginaRestante = 0;

	while(tamMaximo - bytesLeidos > tam_heap_metadata) {
		posUltimoHeapMetadata = offset;
		paginaUltimoHeapMetadata = nroPagina;
		leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset, paginas, &nroPagina);
	}

	if(heapMetadata->estaLibre && nroPagina > paginaUltimoHeapMetadata) {
		tamanioPaginaRestante = tamPagina - posUltimoHeapMetadata%tamPagina;

		if(tamanioPaginaRestante>tam_heap_metadata)
		{
		heapMetadata->offset = tamanioPaginaRestante - tam_heap_metadata; // ver de restar uno
		escribirUnHeapMetadata(paginas, paginaUltimoHeapMetadata, heapMetadata, &posUltimoHeapMetadata, tamanioPaginaRestante);
		}

		liberarPaginas(idProceso, paginaUltimoHeapMetadata, segmento);
	}

}

int32_t procesarFree(char* idProceso, uint32_t posicionSegmento) {

	char msj[250];
	int retorno = -1;

	if(poseeSegmentos(idProceso)) {

		int bytesLiberados = 0;
		t_list* segmentos;
		t_segmento* segmento;
		t_list* paginas;

		pthread_mutex_lock(&mutex_diccionario);
		segmentos = dictionary_get(diccionarioProcesos, idProceso);
		pthread_mutex_unlock(&mutex_diccionario);

		segmento = obtenerSegmento(segmentos, posicionSegmento); // ver de hacer validacion por el nulo

		if(segmento->esCompartido)
		{
			sprintf(msj, "El Proceso %s intento liberar un Heap Metadata compartido en la posicion %d", idProceso, posicionSegmento);
			loggearInfo(msj);
			return 1;
		}
		paginas = segmento->paginas;

		uint32_t posicionMemoria = obtenerDireccionMemoria(paginas, posicionSegmento - segmento->posicionInicial);

		posicionMemoria = obtenerPosicionPreviaHeap(paginas, posicionMemoria);

		bytesLiberados = liberarUnHeapMetadata(paginas, posicionMemoria);

		if(bytesLiberados > 0) {
			sprintf(msj, "El Proceso %s libero %d bytes en la posicion %d", idProceso, (int)(bytesLiberados - tam_heap_metadata), posicionSegmento);
			retorno = 1;
		}

		switch(bytesLiberados) {
			case HM_NO_EXISTENTE:
				sprintf(msj, "El Proceso %s intento liberar un Heap Metadata no existente en la posicion %d", idProceso, posicionSegmento);
				break;
			case HM_YA_LIBERADO:
				sprintf(msj, "El Proceso %s intento liberar un Heap Metadata que ya estaba libre en la posicion %d", idProceso, posicionSegmento);
				break;
		}

		loggearInfo(msj);

		int bytesAgrupados = defragmentarSegmento(segmento);

		if(bytesAgrupados > 0) {
			int segundosBytesAgrupados = defragmentarSegmento(segmento);

			if(segundosBytesAgrupados > 0) {
				bytesAgrupados = segundosBytesAgrupados;
			}

			sprintf(msj, "Para el proceso %s, se agruparon %d bytes libres contiguos", idProceso, bytesAgrupados);
			loggearInfo(msj);
		}

		compactarSegmento(idProceso, segmento);

		return retorno;
	}

	sprintf(msj, "El Proceso %s no ha realizado el init correspondiente", idProceso);
	loggearWarning(msj);

	return retorno;

}

void* procesarGet(char* idProceso, uint32_t posicionSegmento, int32_t tamanio) {

	char msj[200];

	if(poseeSegmentos(idProceso))
	{
		t_list * paginas;
		t_list* segmentos;
		t_segmento* segmento;


		pthread_mutex_lock(&mutex_diccionario);
		segmentos = dictionary_get(diccionarioProcesos,idProceso);
		pthread_mutex_unlock(&mutex_diccionario);

		segmento = obtenerSegmento(segmentos, posicionSegmento); // ver de hacer validacion por el nulo
		paginas = segmento->paginas;

		uint32_t posicionPosteriorHeap = obtenerDireccionMemoria(paginas,posicionSegmento  - segmento->posicionInicial);

		uint32_t posicionAnteriorHeap = obtenerPosicionPreviaHeap(paginas,posicionPosteriorHeap);

		void * buffer = malloc(tamanio);

		int bytesLeidos = leerUnHeapMetadata(paginas, posicionAnteriorHeap,posicionPosteriorHeap, &buffer, tamanio);

		if(bytesLeidos > 0)
		{
			sprintf(msj, "El Proceso %s leyo %d bytes de la posicion %d", idProceso,bytesLeidos,posicionSegmento);
		}

		switch(bytesLeidos)
		{
		case HM_NO_EXISTENTE:
			sprintf(msj, "El Proceso %s intento leer de un HM no existente en la posicion %d", idProceso, posicionSegmento);
			break;
		case HM_YA_LIBERADO:
			sprintf(msj, "El Proceso %s intento leer de un HM que estaba libre en la posicion %d", idProceso, posicionSegmento);
			break;
		case TAMANIO_SOBREPASADO:
			sprintf(msj, "El Proceso %s intento leer mas bytes (%d) de los permitidos en la posicion %d", idProceso,tamanio,posicionSegmento);
			break;
		}

		loggearInfo(msj);

		return buffer;

	}
	sprintf(msj, "El Proceso %s no ah realizado el init correspondiente", idProceso);
	loggearInfo(msj);

	return NULL;

}

int procesarCpy(char* idProceso, uint32_t posicionSegmento, int32_t tamanio, void* contenido) {

	char msj[250];
	int retorno = -1;

	if(poseeSegmentos(idProceso))
		{

			pthread_mutex_lock(&mutex_diccionario);
			t_list * segmentos = dictionary_get(diccionarioProcesos,idProceso);
			pthread_mutex_unlock(&mutex_diccionario);

			t_segmento * segmento = obtenerSegmento(segmentos, posicionSegmento); // ver de hacer validacion por el nulo
			t_list * paginas = segmento->paginas;

			if(paginas == NULL)
			{
				sprintf(msj, "El Proceso %s intento escribir fuera del segmento en la direccion %d", idProceso, posicionSegmento);
				loggearInfo(msj);
				return -1;
			}

			uint32_t posicionPosteriorHeap = obtenerDireccionMemoria(paginas,posicionSegmento  - segmento->posicionInicial);

			uint32_t posicionAnteriorHeap = obtenerPosicionPreviaHeap(paginas,posicionPosteriorHeap);

			//void * buffer = malloc(tamanio);

			int bytesEscritos = escribirDatosHeapMetadata(paginas,posicionAnteriorHeap, posicionPosteriorHeap, &contenido, tamanio);

			if(bytesEscritos > 0)
			{
				sprintf(msj, "El Proceso %s escribio %d bytes en la posicion %d", idProceso,bytesEscritos,posicionSegmento);
				retorno = 0;
			}

			switch(bytesEscritos)
			{
			case HM_NO_EXISTENTE:
				sprintf(msj, "El Proceso %s intento escribir en un HM no existente en la posicion %d", idProceso, posicionSegmento);
				break;
			case HM_YA_LIBERADO:
				sprintf(msj, "El Proceso %s intento escribir en un HM que estaba libre en la posicion %d", idProceso, posicionSegmento);
				break;
			case TAMANIO_SOBREPASADO:
				sprintf(msj, "El Proceso %s intento escribir mas bytes (%d) de los permitidos en la posicion %d", idProceso,tamanio,posicionSegmento);
				break;
			}

			loggearInfo(msj);
		}
	else{
	sprintf(msj, "El Proceso %s no ah realizado el init correspondiente", idProceso);
	loggearInfo(msj);
	}

	return retorno;

}

uint32_t procesarMap(char* idProceso, char* path, int32_t tamanio, int32_t flag) {

	char msj[450];
	uint32_t posicionRetorno = 0;
	if(existeEnElDiccionario(idProceso))
	{
	char aux[35];
	if(flag == MUSE_MAP_SHARED)
		strcpy(aux,"MUSE MAP SHARED");
	else
		strcpy(aux,"MUSE MAP PRIVATE");

	int cantidadFrames = obtenerCantidadMarcos(tamPagina, tamanio + tam_heap_metadata);
	t_archivo_compartido * unArchivoCompartido = NULL;


	if(flag == MUSE_MAP_SHARED)
	{
		unArchivoCompartido = obtenerArchivoCompartido(path);

		if(unArchivoCompartido) // Entonces ya existe en memoria! Solo hay que agregarlo en las estructuras administrativas. Si no hay que agregarlo en memoria
		{
			agregarArchivoLista(path,unArchivoCompartido);
			posicionRetorno = agregarPaginasSinMemoria(idProceso,unArchivoCompartido,cantidadFrames);
			sprintf(msj, "El Proceso %s mapeo el archivo compartido %s en la posicion %u. El archivo ya se encontraba en memoria compartida", idProceso, path,posicionRetorno);
			loggearInfo(msj);
			return posicionRetorno;
		}
	}

	posicionRetorno = analizarSegmento(idProceso, tamanio, cantidadFrames, true);

	void * buffer = obtenerDatosArchivo(path,tamanio);

	if(!buffer)
	{
		sprintf(msj, "El Proceso %s intento leer el archivo %s no existente en el FileSystem", idProceso, path);
		loggearWarning(msj);
		return 0;
	}

	t_list * segmentos;
	t_segmento* segmento;

	pthread_mutex_lock(&mutex_diccionario);
	segmentos = dictionary_get(diccionarioProcesos,idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	segmento = obtenerSegmento(segmentos, posicionRetorno); // ver de hacer validacion por el nulo

	t_list * paginas = segmento->paginas;
	t_pagina * unaPagina;

	uint32_t posicionMemoria = obtenerDireccionMemoria(paginas, posicionRetorno - segmento->posicionInicial);

	escribirDatosHeap(paginas, posicionMemoria, &buffer, tamanio);

	/*
	 * Aca la idea seria agregar al diccionario!
	 */

	if(flag == MUSE_MAP_SHARED)
		{
			unArchivoCompartido = agregarArchivoLista(path,unArchivoCompartido); // me devuelve el nuevo archivo compartido

			unArchivoCompartido->marcosMapeados = malloc(sizeof(int32_t)*cantidadFrames);
			unArchivoCompartido->nroPaginaSwap = malloc(sizeof(int32_t)*cantidadFrames);

			for(int i = 0; i < cantidadFrames;i++)
			{
				unaPagina = list_get(paginas,i);
				*(unArchivoCompartido->marcosMapeados + i) = unaPagina->nroMarco;
				*(unArchivoCompartido->nroPaginaSwap + i) = unaPagina->nroPaginaSwap;
			}
		}

		t_segmento* unSegmento = obtenerUnSegmento(idProceso, posicionRetorno);
		unSegmento->archivo = strdup(path);

		sprintf(msj,"El proceso %s escribio %d bytes en la posicion %d para el archivo %s con el flag %s",idProceso,tamanio,posicionRetorno,path,aux);
	}else
	{
		sprintf(msj, "El Proceso %s no ah realizado el init correspondiente", idProceso);
	}

	loggearInfo(msj);

	free(path);

	return posicionRetorno;

}

int procesarSync(char* idProceso, uint32_t posicionMemoria, int32_t tamanio) {


	char msj[450];
	int retorno = -1;

	if(existeEnElDiccionario(idProceso))
	{
	void * buffer = procesarGet(idProceso, posicionMemoria, tamanio);
	t_segmento* unSegmento = obtenerUnSegmento(idProceso, posicionMemoria);
	retorno = copiarDatosEnArchivo(unSegmento->archivo, tamanio, buffer);
	sprintf(msj,"El Proceso %s descargo %d bytes en el archivo %s",idProceso,tamanio,unSegmento->archivo);
	}
	else
	{
		sprintf(msj, "El Proceso %s no ah realizado el init correspondiente", idProceso); // ver de pasar la validacion al lado del cliente
	}

	return retorno;

}


int procesarUnmap(char* idProceso, uint32_t posicionMemoria) {

	char msj[350];

	pthread_mutex_lock(&mutex_diccionario);
	t_list * segmentos = dictionary_get(diccionarioProcesos, idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	t_segmento* unSegmento = obtenerSegmento(segmentos, posicionMemoria);

	int cantidadParticipantes = obtenerCantidadParticipantes(unSegmento->archivo);

	liberarConUnmap(idProceso,unSegmento,cantidadParticipantes==1);

	reducirArchivoCompartido(unSegmento->archivo);

	free(unSegmento->archivo);

	/*if(unSegmento->id_segmento == list_size(segmentos)-1)
	{
		sprintf(msj,"Unmap resulta en que el segmento %d es liberado de la lista del proceso %s",unSegmento->id_segmento,idProceso);
		list_remove_and_destroy_element(segmentos,unSegmento->id_segmento,(void*)free); // falta analizar que pasa si es el unico/ultimo elemento de la lista => PENDIENTE (aunque no es un requerimiento)
	}else
	{
		sprintf(msj,"Unmap ejecutado correctamente para el proceso %s direccion %d",idProceso,unSegmento->id_segmento);
	}*/

	sprintf(msj,"Unmap ejecutado correctamente para el proceso %s direccion [%d-%d]",idProceso,unSegmento->id_segmento,unSegmento->tamanio);
	loggearInfo(msj);

	return 0;
}

int procesarClose(char* idProceso) {

 	char msj[120];
	t_list* segmentos;
	pthread_mutex_lock(&mutex_diccionario);
	segmentos = dictionary_get(diccionarioProcesos, idProceso);
	pthread_mutex_unlock(&mutex_diccionario);
	if(segmentos != NULL) {
		if(!list_is_empty(segmentos)) {
			void liberarSegmento(t_segmento* unSegmento) {
				if(unSegmento)
				{
						if(unSegmento->paginas)
						{
							void liberarPaginas(t_pagina* unaPagina) {
								liberarMarcoBitarray(unaPagina->nroMarco);
								free(unaPagina);
							}

							list_destroy_and_destroy_elements(unSegmento->paginas, (void*)liberarPaginas);
						}
						if(unSegmento->esCompartido)
							reducirArchivoCompartido(unSegmento->archivo);
					free(unSegmento);
				}
			}

			list_destroy_and_destroy_elements(segmentos, (void*)liberarSegmento);
		}
	}
	pthread_mutex_lock(&mutex_diccionario);
	dictionary_remove(diccionarioProcesos, idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	sprintf(msj, "Proceso %s eliminado del diccionario de procesos", idProceso);
	loggearInfo(msj);

	return 0;

}

/*
 * FUNCIONES MACRO DE OPERACIONES
 */

// MACRO DE MALLOC

uint32_t analizarSegmento (char* idProceso, int tamanio, int cantidadFrames, bool esCompartido) {
	//Que pasa si en el medio de esta operacion el mismo proceso mediante otro hilo me inserta otro segmento al mismo tiempo = PROBLEMAS

	uint32_t direccionRetorno = 0;
	t_list* listaSegmentos;
	int nroSegmento = 0;

	if(!poseeSegmentos(idProceso)) // => Es el primer malloc
	{
		listaSegmentos = list_create();
		crearSegmento(idProceso, tamanio, cantidadFrames, listaSegmentos, 0, esCompartido, 0);
		direccionRetorno =  tam_heap_metadata;
	}
	else
	{
		t_segmento* ultimoSegmento;
		uint32_t ultimaPosicionSegmento;

		pthread_mutex_lock(&mutex_diccionario);
		listaSegmentos = dictionary_get(diccionarioProcesos, idProceso);
		pthread_mutex_unlock(&mutex_diccionario);

		ultimoSegmento = list_get(listaSegmentos, list_size(listaSegmentos) - 1);

		ultimaPosicionSegmento = ultimoSegmento->posicionInicial + ultimoSegmento->tamanio;
		nroSegmento = ultimoSegmento->id_segmento + 1;

		if(ultimoSegmento->esCompartido || esCompartido)
		{
			crearSegmento(idProceso, tamanio, cantidadFrames, listaSegmentos,nroSegmento, esCompartido, ultimaPosicionSegmento); // HACER
			direccionRetorno = ultimaPosicionSegmento + tam_heap_metadata;
		}
		else
			direccionRetorno = completarSegmento(idProceso, ultimoSegmento, tamanio);

	}
	return direccionRetorno;

}

#endif /* MUSEOPERACIONES_H_ */
