/*
 * MuseMemoriaCompartida.h
 *
 *  Created on: Dec 14, 2019
 *      Author: agus
 */

#ifndef MUSEMEMORIACOMPARTIDA_H_
#define MUSEMEMORIACOMPARTIDA_H_

#include "MuseRutinasFree.h"


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
