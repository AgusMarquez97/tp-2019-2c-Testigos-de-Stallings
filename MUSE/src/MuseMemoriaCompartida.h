/*
 * MuseMemoriaCompartida.h
 *
 *  Created on: Dec 14, 2019
 *      Author: agus
 */

#ifndef MUSEMEMORIACOMPARTIDA_H_
#define MUSEMEMORIACOMPARTIDA_H_

#include "MuseCpyGet.h"

uint32_t analizarMap(char* idProceso, char* path, int32_t tamanio, int32_t flag)
{
		uint32_t posicionRetorno = 0;
		char msj[450];
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
				posicionRetorno = agregarPaginasSinMemoria(path,idProceso,unArchivoCompartido,cantidadFrames);
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

		segmento = obtenerSegmento(segmentos, posicionRetorno);

		t_list * paginas = segmento->paginas;

		int nroPaginaActual = obtenerNroPagina(paginas,posicionRetorno - segmento->posicionInicial);
		t_pagina * unaPagina = list_get(paginas,nroPaginaActual);

		uint32_t posicionMemoria = obtenerOffsetPosterior(paginas,posicionRetorno - segmento->posicionInicial,nroPaginaActual);

		escribirDatosHeap(paginas,nroPaginaActual, posicionMemoria, &buffer, tamanio);
		free(buffer);

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

			segmento->archivo = strdup(path);

			sprintf(msj,"El proceso %s escribio %d bytes en la posicion %d para el archivo %s con el flag %s",idProceso,tamanio,posicionRetorno,path,aux);
			loggearInfo(msj);

			return posicionRetorno;
}

int analizarSync(char* idProceso, uint32_t posicionSegmento, int32_t tamanio)
{
		int retorno;
		char msj[450];
		void * buffer = procesarGet(idProceso, posicionSegmento, tamanio);
		if(!buffer)
			return -1;//agregar log
		t_segmento* unSegmento = obtenerUnSegmento(idProceso, posicionSegmento);
		retorno = copiarDatosEnArchivo(unSegmento->archivo, tamanio, buffer);
		free(buffer);
		if(retorno == -1)
			return -1;
		sprintf(msj,"El Proceso %s descargo %d bytes en el archivo %s",idProceso,tamanio,unSegmento->archivo);
		loggearInfo(msj);
		return retorno;
}

int analizarUnmap(char* idProceso, uint32_t posicionSegmento)
{
	char msj[350];
	pthread_mutex_lock(&mutex_diccionario);
	t_list * segmentos = dictionary_get(diccionarioProcesos, idProceso);
	pthread_mutex_unlock(&mutex_diccionario);

	t_segmento* unSegmento = obtenerSegmento(segmentos, posicionSegmento);

	if(!unSegmento)
			return -1;

	int cantidadParticipantes = obtenerCantidadParticipantes(unSegmento->archivo);

	liberarConUnmap(idProceso,unSegmento,cantidadParticipantes==1);

	reducirArchivoCompartido(unSegmento->archivo);

	free(unSegmento->archivo);

	unSegmento->archivo = NULL;

	sprintf(msj,"Unmap ejecutado correctamente para el proceso %s direccion [%d-%d]",idProceso,unSegmento->id_segmento,unSegmento->tamanio);
	loggearInfo(msj);

	return 0;
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
	pthread_mutex_lock(&mutex_lista_archivos);
	t_archivo_compartido * unArchivo =  list_find(listaArchivosCompartidos,(void*)existencia); // En este caso no me deberia importar lockear
	pthread_mutex_unlock(&mutex_lista_archivos);
	return unArchivo;
}

uint32_t agregarPaginasSinMemoria(char * path, char * idProceso,t_archivo_compartido * unArchivoCompartido,int cantidadFramesTeoricos)
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
	segmentoNuevo = crearSegmentoSinMemoria(path,paginas, idSegmento, posicionInicial, cantidadFramesTeoricos);

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
		unaPagina->nroPaginaSwap = *(unArchivoCompartido->nroPaginaSwap + i);
		unaPagina->uso = 1;
		unaPagina->modificada = 0;
		unaPagina->esCompartida = true;

		list_add(listaPaginas,unaPagina);

		pthread_mutex_lock(&mutex_algoritmo_reemplazo);
		list_add(listaPaginasClockModificado,unaPagina);
		pthread_mutex_unlock(&mutex_algoritmo_reemplazo);
	}
	return listaPaginas;
}

t_segmento * crearSegmentoSinMemoria(char * path,t_list * listaPaginas,int idSegmento,uint32_t posicionInicial,int cantidadFramesTeoricos)
{
	t_segmento * segmentoNuevo = malloc(sizeof(*segmentoNuevo));

	segmentoNuevo->esCompartido=true;
	segmentoNuevo->id_segmento=idSegmento;
	segmentoNuevo->posicionInicial = posicionInicial;
	segmentoNuevo->tamanio = cantidadFramesTeoricos*tamPagina;
	segmentoNuevo->paginas = listaPaginas;
	segmentoNuevo->archivo = strdup(path);

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

			close(fd_num);

			return 0;

}


void liberarConUnmap(char * idProceso, t_segmento * unSegmento,bool sinParticipantes)
{

	char msj[450];
	char aux[100];

	if(list_size(unSegmento->paginas)==1) {
		sprintf(msj, "Para el proceso %s, se ha liberado la página ", idProceso);
	} else {
		sprintf(msj, "Para el proceso %s, se han liberado las páginas [ ", idProceso);
	}

	void liberarPagina(t_pagina* pagina) {
			pthread_mutex_lock(&mutex_lista_paginas);
			bool condicion(t_pagina * unaPaginaAlgoritmo)
			{
				return unaPaginaAlgoritmo == pagina;
			}
			list_remove_by_condition(listaPaginasClockModificado,(void*)condicion);
			pthread_mutex_unlock(&mutex_lista_paginas);

			if(sinParticipantes)
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

	void destructor(t_archivo_compartido * unArchivoCompartido)
	{
		free(unArchivoCompartido->marcosMapeados);
		free(unArchivoCompartido->nroPaginaSwap);
		free(unArchivoCompartido->nombreArchivo);
		free(unArchivoCompartido);
	}

	pthread_mutex_lock(&mutex_lista_archivos);
	list_remove_and_destroy_by_condition(listaArchivosCompartidos,(void*)condicion,(void*)destructor);
	pthread_mutex_unlock(&mutex_lista_archivos);
}

int obtenerCantidadParticipantes(char * path)
{
	t_archivo_compartido * unArchivoCompartido = obtenerArchivoCompartido(path);

	return unArchivoCompartido->nroParticipantes;
}


#endif /* MUSEMEMORIACOMPARTIDA_H_ */
