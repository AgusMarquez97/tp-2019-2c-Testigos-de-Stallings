#ifndef MUSEAUXILIARES_H_
#define MUSEAUXILIARES_H_

#include "MUSE.h"

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

t_pagina* obtenerPagina(t_list* paginas, uint32_t posicionMemoria) {

	bool paginaCorrespondiente(t_pagina* pagina) {
		return (posicionMemoria >= pagina->nroPagina*tamPagina
				&& posicionMemoria <= (pagina->nroPagina + 1) * tamPagina);
	}
	return list_find(paginas, (void*)paginaCorrespondiente);

}

bool poseeSegmentos(char* idProceso) {

	return (dictionary_get(diccionarioProcesos, idProceso) != NULL);

}


void agregarPaginas(t_list** listaPaginas, int cantidadMarcos, int nroUltimaPagina) { // Analizar de agregar log de cada pagina que se pide por proceso

	t_pagina * pagina;

	for(int i = 0; i < cantidadMarcos; i++) {
		pagina = malloc(sizeof(pagina));

		pagina->nroMarco = asignarMarcoLibre(); // Agregar logica del algoritmo de reemplazo de pags
		pagina->nroPagina = i + nroUltimaPagina;

		list_add(*listaPaginas, pagina);
	}

}

t_list* crearListaPaginas(int tamanio, int cantidadMarcos) {

	t_list* listaPaginas = list_create();

	agregarPaginas(&listaPaginas,cantidadMarcos,0);

	int primerMarco = ((t_pagina*)list_get(listaPaginas, 0))->nroMarco;

	escribirHeapMetadata(listaPaginas, primerMarco*tamPagina, tamanio,false);

	return listaPaginas;

}

t_segmento* instanciarSegmento(int tamanio, int cantidadFrames, int idSegmento, bool esCompartido, int posicionInicial) {

	t_list* listaPaginas = crearListaPaginas(tamanio, cantidadFrames);

	t_segmento * segmento = malloc(sizeof(t_segmento));

	segmento->id_segmento = idSegmento;
	segmento->esCompartido = esCompartido;
	segmento->posicionInicial = posicionInicial;
	segmento->tamanio = cantidadFrames * tamPagina;
	segmento->paginas = listaPaginas;

	return segmento;

}

void crearSegmento(char* idProceso, int tamanio, int cantidadMarcos, t_list* listaSegmentos, int idSegmento, bool esCompartido, int posicionInicial) {

		t_segmento* segmento = instanciarSegmento(tamanio, cantidadMarcos, idSegmento, esCompartido, posicionInicial);

		list_add(listaSegmentos, segmento);

		pthread_mutex_lock(&mutex_diccionario);
		dictionary_remove(diccionarioProcesos, idProceso);
		dictionary_put(diccionarioProcesos, idProceso, listaSegmentos);
		pthread_mutex_unlock(&mutex_diccionario);

}

uint32_t completarSegmento(char * idProceso,t_segmento* segmento, int tamanio) {

	int cantPaginas = list_size(segmento->paginas);
	int tamMaximo = tamPagina * cantPaginas;
	t_pagina* unaPagina = list_get(segmento->paginas, 0);
	int offset = unaPagina->nroMarco * tamPagina;
	int bytesLeidos = 0;
	t_heap_metadata* heapMetadata = malloc(tam_heap_metadata); // liberar

	int sobrante = 0;

	int contador = 0;
	int bytesLeidosPagina = 0;
	int offsetAnterior = 0;


	while(tamMaximo - bytesLeidos > tam_heap_metadata) {
		offsetAnterior = offset;
		leerHeapMetadata(&heapMetadata, &bytesLeidos, &bytesLeidosPagina, &offset,segmento->paginas,&contador);

		if(heapMetadata->estaLibre && heapMetadata->offset >= (tamanio + tam_heap_metadata)) {

			bool tieneUnoSiguiente = existeHM(segmento->paginas, offset);

			if(tieneUnoSiguiente)
				escribirHeapMetadata(segmento->paginas, offsetAnterior, tamanio,tam_heap_metadata + heapMetadata->offset); // validado
			else
				escribirHeapMetadata(segmento->paginas, offsetAnterior, tamanio,false); // validado
			return segmento->posicionInicial + offsetAnterior + tam_heap_metadata;
		}

	}

	sobrante = (tamMaximo - bytesLeidos);

	if(heapMetadata->estaLibre) {
		sobrante = heapMetadata->offset + tam_heap_metadata; // me sobro un hm entero
		offset = offsetAnterior; // Pongo el offset en la primera posicion libre
	}

	int nuevaCantidadFrames = obtenerCantidadMarcos(tamPagina, tamanio + tam_heap_metadata - sobrante); // Frames necesarios para escribir en memoria

	int retorno = estirarSegmento(idProceso, segmento, tamanio, nuevaCantidadFrames, offset, sobrante);

	free(heapMetadata); // testear!!

	return segmento->posicionInicial + retorno;

}

// offset anterior al heap!
int estirarSegmento(char* idProceso, t_segmento* segmento, int tamanio, int nuevaCantidadFrames, int offset, int sobrante) {

	int retorno = 0;

	t_list* listaPaginas = segmento->paginas;
	int nroUltimaPagina = list_size(listaPaginas);

	agregarPaginas(&listaPaginas, nuevaCantidadFrames, nroUltimaPagina);

	escribirHeapMetadata(listaPaginas,offset,tamanio,false); // Escribir el heap nuevo en memoria. Considera heap partido

	segmento->tamanio = list_size(listaPaginas) * tamPagina; // ver si es necesario un mutex por cada operacion con el segmento

	if(sobrante > tam_heap_metadata)
	{
		retorno = offset + tam_heap_metadata;
	}
	else
	{
		t_pagina * unaPagina = list_get(listaPaginas,nroUltimaPagina);
		retorno = unaPagina->nroMarco*tamPagina + (tam_heap_metadata - sobrante);
	}

	return retorno;
}


int cantidadPaginasPedidas(int offset) {

	return (int) offset / tamPagina; // redondea al menor numero para abajo

}

void* leerDeMemoria(int posicionInicial, int tamanio) {

	void* buffer = malloc(tamanio);
	pthread_mutex_lock(&mutex_memoria);
	memcpy(buffer, memoria + posicionInicial, tamanio);
	pthread_mutex_unlock(&mutex_memoria);

	return buffer;

}

void escribirEnMemoria(void* contenido, int posicionInicial, int tamanio) {

	pthread_mutex_lock(&mutex_memoria);
	memcpy(memoria + posicionInicial, contenido, tamanio);
	pthread_mutex_unlock(&mutex_memoria);

}

void liberarMarcoBitarray(int nroMarco) {

	pthread_mutex_lock(&mutex_marcos_libres);
	bitarray_clean_bit(marcosMemoriaPrincipal, nroMarco);
	pthread_mutex_unlock(&mutex_marcos_libres);

}

bool estaLibreMarco(int nroMarco) {

	return bitarray_test_bit(marcosMemoriaPrincipal, nroMarco) == 0;

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

	return -1;

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

void liberarPaginas(char* idProceso, int nroPagina, t_segmento* segmento) {

	char msj[100];
	char aux[30];

	t_list * paginas = segmento->paginas;

	if((nroPagina + 1) == list_size(paginas)) {
		sprintf(msj, "Para el proceso %s, se ha liberado la página ", idProceso);
	} else {
		sprintf(msj, "Para el proceso %s, se han liberado las páginas [ ", idProceso);
	}

	bool buscarPagina(t_pagina* pagina) {
		return (pagina->nroPagina <= nroPagina);
	}

	t_list* lista_aux = list_filter(paginas, (void*)buscarPagina);

	void liberarPagina(t_pagina* pagina) {
		if(pagina->nroPagina > nroPagina) {
			sprintf(aux, "%d ",pagina->nroPagina);
			strcat(msj, aux);
			liberarMarcoBitarray(pagina->nroMarco);
			free(pagina);
		}
	}

	list_destroy_and_destroy_elements(paginas, (void*)liberarPagina);

	segmento->paginas = list_duplicate(lista_aux); //OJO! POSIBLE ML
	segmento->tamanio = tamPagina*list_size(segmento->paginas);

	strcat(msj, "]");

	loggearInfo(msj);

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

void * obtenerDatosArchivo(char * path, int tamanio)
{
	void * buffer = malloc(tamanio);
	int tamanioArchivo = 0;

	FILE * fd = fopen(path,"r+");
		if(!fd)
			return NULL;

	struct stat statbuf = {0};
	stat(path,&statbuf);

	int fd_num = fileno(fd);
	tamanioArchivo = statbuf.st_size;

	void * bufferAuxiliar = mmap(NULL,tamanioArchivo,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd_num,0);

	/*
	 * Validar con franco
	 */

	if(tamanio <= tamanioArchivo)
		memcpy(buffer,bufferAuxiliar,tamanio);
	else
	{
		memcpy(buffer,bufferAuxiliar,tamanioArchivo);
		memset(buffer + tamanioArchivo,0,tamanio-tamanioArchivo);
	}

	munmap(buffer,tamanioArchivo);

	fclose(fd);

	return buffer;
}

t_archivo_compartido * agregarArchivoLista(char * unArchivo, t_archivo_compartido * archivoCompartido)
{
	t_archivo_compartido * unArchivoCompartido = NULL;
	if(archivoCompartido!=NULL)
	{
	pthread_mutex_lock(&mutex_lista_archivos);
	archivoCompartido->nroParticipantes++;
	pthread_mutex_unlock(&mutex_lista_archivos);
	}
	else
	{
	unArchivoCompartido = malloc(sizeof(*unArchivoCompartido));

	unArchivoCompartido->nombreArchivo = strdup(unArchivo);
	unArchivoCompartido->nroParticipantes = 1;

	pthread_mutex_lock(&mutex_lista_archivos);
	list_add(listaArchivosCompartidos,unArchivoCompartido);
	pthread_mutex_unlock(&mutex_lista_archivos);
	}
	return unArchivoCompartido;
}

t_archivo_compartido * obtenerArchivoCompartido(char * path)
{
	bool existencia(t_archivo_compartido * unArchivoComp)
	{
		return strcmp(unArchivoComp->nombreArchivo,path) == 0;
	}
	t_archivo_compartido * unArchivo =  list_find(listaArchivosCompartidos,(void*)existencia); // En este caso no me deberia importar lockear
	return unArchivo;
}

uint32_t agregarPaginasSinMemoria(char * idProceso,t_archivo_compartido * unArchivoCompartido,int cantidadFramesTeoricos)
{
	t_list * listaSegmentos;
	t_segmento * segmentoNuevo;
	uint32_t posicionInicial = 0;
	int idSegmento = 0;

	if(poseeSegmentos(idProceso))
	{
		pthread_mutex_lock(&mutex_diccionario);
		listaSegmentos = dictionary_get(diccionarioProcesos,idProceso);
		pthread_mutex_unlock(&mutex_diccionario);

		t_segmento * ultimoSegmento = list_get(listaSegmentos,list_size(listaSegmentos)-1);

		posicionInicial = ultimoSegmento->posicionInicial + ultimoSegmento->tamanio;
		idSegmento = ultimoSegmento->id_segmento + 1;
	}
	else
	{
		listaSegmentos = list_create();
	}

	t_list * paginas = crearPaginasSinMemoria(unArchivoCompartido, cantidadFramesTeoricos);
	segmentoNuevo = crearSegmentoSinMemoria(paginas, idSegmento, posicionInicial, cantidadFramesTeoricos);

	list_add(listaSegmentos,segmentoNuevo);

	pthread_mutex_lock(&mutex_diccionario);
	dictionary_remove(diccionarioProcesos, idProceso);
	dictionary_put(diccionarioProcesos, idProceso, listaSegmentos);
	pthread_mutex_unlock(&mutex_diccionario);

	return posicionInicial + tam_heap_metadata; // al principio nunca estara partido
}

t_list * crearPaginasSinMemoria(t_archivo_compartido * unArchivoCompartido,int cantidadFramesTeoricos)
{
	t_list * listaPaginas = list_create();

	for(int i = 0;i < cantidadFramesTeoricos;i++)
	{
		t_pagina * unaPagina = malloc(sizeof(*unaPagina));

		unaPagina->nroPagina = i;
		unaPagina->nroMarco = *(unArchivoCompartido->marcosMapeados + i);

		list_add(listaPaginas,unaPagina);
	}
	return listaPaginas;
}

t_segmento * crearSegmentoSinMemoria(t_list * listaPaginas,int idSegmento,uint32_t posicionInicial,int cantidadFramesTeoricos)
{
	t_segmento * segmentoNuevo = malloc(sizeof(*segmentoNuevo));

	segmentoNuevo->esCompartido=true;
	segmentoNuevo->id_segmento=idSegmento;
	segmentoNuevo->posicionInicial = posicionInicial;
	segmentoNuevo->tamanio = cantidadFramesTeoricos*tamPagina;
	segmentoNuevo->paginas = listaPaginas;

	return segmentoNuevo;
}

int copiarDatosEnArchivo(char * path, int tamanio, void * buffer)
{
			FILE * fd = fopen(path,"r+");
				if(!fd)
					return -1;

			struct stat statbuf = {0};
			stat(path,&statbuf);

			int fd_num = fileno(fd);

			ftruncate(fd_num,tamanio); // lo que habia volo!

			void * bufferAuxiliar = mmap(NULL,tamanio,PROT_READ|PROT_WRITE,MAP_SHARED,fd_num,0);

			memcpy(bufferAuxiliar,buffer,tamanio);

			msync(bufferAuxiliar,tamanio,MS_SYNC);

			munmap(bufferAuxiliar,tamanio);

			return 0;

}


void liberarConUnmap(char * idProceso, t_segmento * unSegmento)
{

	char msj[100];
	char aux[30];

	if(list_size(unSegmento->paginas)==1) {
		sprintf(msj, "Para el proceso %s, se ha liberado la página ", idProceso);
	} else {
		sprintf(msj, "Para el proceso %s, se han liberado las páginas [ ", idProceso);
	}

	void liberarPagina(t_pagina* pagina) {
				liberarMarcoBitarray(pagina->nroMarco); // Agregar validacion para liberar memoria virtual tambien
				sprintf(aux, "%d ",pagina->nroPagina);
				strcat(msj, aux);
				free(pagina);
		}
	list_destroy_and_destroy_elements(unSegmento->paginas, (void*)liberarPagina);

	strcat(msj, "]");

	unSegmento->paginas=NULL;

	loggearInfo(msj);
}

void reducirArchivoCompartido(char * path)
{

	bool condicion(t_archivo_compartido * unArchivoCompartido)
	{
		if(strcmp(unArchivoCompartido->nombreArchivo,path) == 0)
		{
			if(unArchivoCompartido->nroParticipantes == 1)
				return true;
			else
				unArchivoCompartido->nroParticipantes--;

		}
			return false;
	}

	pthread_mutex_lock(&mutex_lista_archivos);
	list_remove_and_destroy_by_condition(listaArchivosCompartidos,(void*)condicion,free);
	pthread_mutex_unlock(&mutex_lista_archivos);
}



#endif /* MUSEAUXILIARES_H_ */
