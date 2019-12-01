#ifndef MUSEOPERACIONES_H_
#define MUSEOPERACIONES_H_

#include "MuseAuxiliares.h"

bool existeEnElDiccionario(char* idProceso) {

	if(diccionarioProcesos) {
		if(!dictionary_is_empty(diccionarioProcesos)) {
			if(dictionary_has_key(diccionarioProcesos, idProceso)) {
				return true;
			}
		}
	}

	return false;
}

int procesarHandshake(char* idProceso) {

	char msj[120];

	if(existeEnElDiccionario(idProceso)) {
		sprintf(msj, "Fallo al realizar el handshake, init duplicado del proceso %s", idProceso);
		loggearError(msj);
		return -1;
	}

	dictionary_put(diccionarioProcesos, idProceso, NULL);
	sprintf(msj, "Handshake exitoso. Proceso %s agregado al diccionario de procesos correctamente", idProceso);
	loggearInfo(msj);

	return 0;

}

uint32_t procesarMalloc(char* idProceso, int32_t tamanio) {

	char msj[120];
	int cantidadFrames;

	if(existeEnElDiccionario(idProceso)) {
		cantidadFrames = obtenerCantidadMarcos(tamPagina, tamanio + tam_heap_metadata);
		return analizarSegmento(idProceso, tamanio, cantidadFrames, false);
	}

	sprintf(msj, "El proceso %s no realizo el init correspondiente", idProceso);
	loggearWarning(msj);

	return 0;

}

bool obtenerDireccionMemoria(char* idProceso, uint32_t posicionMemoria, int* base, int* offset) {

	int i = 0;
	char msj[120];

	if(existeEnElDiccionario(idProceso)) {
		t_segmento* segmentoBuscado = obtenerSegmento((t_list*)dictionary_get(diccionarioProcesos, idProceso), posicionMemoria);
		t_pagina* paginaBuscada = obtenerPagina(segmentoBuscado->paginas, posicionMemoria);

		while(tamPagina * i <= posicionMemoria) {
			*offset = posicionMemoria - tamPagina * i;
			i++;
		}

		*base = paginaBuscada->nroMarco * tamPagina;
		return true;
	}

	sprintf(msj, "El proceso %s no realizo el init correspondiente", idProceso);
	loggearWarning(msj);
	return false;

}

int32_t procesarFree(char* idProceso, uint32_t posicionMemoria) {

	char msj[120];
	int base = 0;
	int offset = 0;

	if(obtenerDireccionMemoria(idProceso, posicionMemoria, &base, &offset)) {
		uint32_t bytesLiberados = liberarBytesMemoria(base, offset);
		sprintf(msj, "El Proceso %s ha liberado %d bytes de memoria", idProceso, bytesLiberados);
		loggearInfo(msj);

		return 1;
	}

	return -1;

}

void* procesarGet(char* idProceso, uint32_t posicionMemoria, int32_t tamanio) {

	int base = 0;
	int offset = 0;

	if(obtenerDireccionMemoria(idProceso, posicionMemoria, &base, &offset)) {
		t_heap_metadata* heapMetadata = obtenerHeapMetadata(base, offset);

		if(!heapMetadata->estaLibre && heapMetadata->offset >= tamanio)
			return leerDeMemoria(base + offset, tamanio);
	}

	return NULL;

}

int procesarCpy(char* idProceso, uint32_t posicionMemoria, int32_t tamanio, void* contenido) {

	int base = 0;
	int offset = 0;

	if(obtenerDireccionMemoria(idProceso, posicionMemoria, &base, &offset)) {
		t_heap_metadata* heapMetadata = obtenerHeapMetadata(base, offset);

		if(!heapMetadata->estaLibre && heapMetadata->offset >= tamanio) {
			escribirEnMemoria(contenido, base + offset, tamanio);
			return 0;
		}
	}

	return -1;

}

uint32_t procesarMap(char* idProceso, void* contenido, int32_t tamanio, int32_t flag) {

	char* pathArchivo = malloc(tamanio + 1);

	memcpy(pathArchivo, contenido, tamanio);
	pathArchivo[tamanio] = 0;
	loggearInfo(pathArchivo);
	free(pathArchivo);

	return 1;

}

int procesarSync(char* idProceso, uint32_t posicionMemoria, int32_t tamanio) {

	int base = 0;
	int offset = 0;

	if(obtenerDireccionMemoria(idProceso, posicionMemoria, &base, &offset)) {
		t_heap_metadata* heapMetadata = obtenerHeapMetadata(base, offset);

		if(!heapMetadata->estaLibre && heapMetadata->offset >= tamanio) {

			return 0;
		}
	}

	return -1;

}

int procesarUnmap(char* idProceso, uint32_t posicionMemoria) {

	return 0;

}

int procesarClose(char* idProceso) {

 	char msj[120];
	t_list* segmentos;

	segmentos = dictionary_get(diccionarioProcesos, idProceso);

	if(segmentos != NULL) {
		if(!list_is_empty(segmentos)) {
			void liberarSegmento(t_segmento* unSegmento) {
				void liberarPaginas(t_pagina* unaPagina) {
					liberarMarcoBitarray(unaPagina->nroMarco);
					free(unaPagina);
				}
				list_iterate(unSegmento->paginas, (void*)liberarPaginas);
				list_destroy(unSegmento->paginas);
				free(unSegmento);
			}
			list_iterate(segmentos, (void*)liberarSegmento);
			list_destroy(segmentos);
		}
	}

	dictionary_remove(diccionarioProcesos, idProceso);

	sprintf(msj, "Proceso %s eliminado del diccionario de procesos", idProceso);
	loggearInfo(msj);

	return 0;

}

#endif /* MUSEOPERACIONES_H_ */

/*
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
*/



