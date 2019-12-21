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
		char msj[450];
		if(existeArchivo(path))
		{
			char * aux = strdup(obtenerFlag(flag));

			int cantidadPaginas = obtenerCantidadMarcos(tamPagina, tamanio);
			t_archivo_compartido * unArchivoCompartido = NULL;


			t_segmento* segmento = crearSegmentoCompartido(idProceso, path, tamanio, cantidadPaginas,flag==MUSE_MAP_SHARED);

			if(flag == MUSE_MAP_SHARED)
			{
				unArchivoCompartido = obtenerArchivoCompartido(path);

				if(unArchivoCompartido) // Entonces ya existe en memoria! ALGUIEN YA LO MAPPEO!
				{
					segmento->paginas = unArchivoCompartido->listaPaginas;
				}
				else
				{
					segmento->paginas = crearPaginasSinMemoria(cantidadPaginas);
					descargarAMemoria(path,segmento->paginas , segmento->posicionInicial, tamanio);
				}
				agregarArchivoLista(path, unArchivoCompartido, segmento->paginas); // si es nulo lo crea, si no, agrega participantes
			}
			else
			{
				segmento->paginas = crearPaginasSinMemoria(cantidadPaginas);
				descargarAMemoria(path,segmento->paginas , segmento->posicionInicial, tamanio);
			}

			segmento->archivo = strdup(path);

			sprintf(msj,"El proceso %s escribio %d bytes en la posicion %d para el archivo %s con el flag %s",idProceso,tamanio,segmento->posicionInicial,path,aux);
			loggearInfo(msj);

			free(aux);

			return segmento->posicionInicial;
		}
		else
		{
			sprintf(msj, "El Proceso %s intento leer el archivo %s no existente en el FileSystem", idProceso, path);
			loggearWarning(msj);
			return 1;
		}
}

int analizarSync(char* idProceso, uint32_t posicionSegmento, int32_t tamanio)
{
		int retorno = 0;
		char msj[450];

		t_segmento* unSegmento = obtenerUnSegmento(idProceso, posicionSegmento);

		t_list * listaPaginasModificadas = obtenerPaginasModificadasLocal(unSegmento->paginas);

		retorno = actualizarArchivo(unSegmento->archivo,unSegmento,posicionSegmento ,tamanio, listaPaginasModificadas);

		list_destroy(listaPaginasModificadas);

		if(retorno == -1)
			return -1;


		sprintf(msj,"El Proceso %s descargo %d bytes en el archivo %s",idProceso,retorno,unSegmento->archivo);
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

	if(unSegmento->tiene_flag_shared)
	{
		reducirArchivoCompartido(idProceso, unSegmento);
	}
	else
	{
		liberarConUnmap(idProceso,unSegmento);
	}

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

t_archivo_compartido * agregarArchivoLista(char * unArchivo, t_archivo_compartido * archivoCompartido, t_list * listaPaginas)
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
	unArchivoCompartido->listaPaginas = listaPaginas; // requiere crear la lista??

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

t_list * crearPaginasSinMemoria(int cantidadFramesTeoricos)
{
	t_list * listaPaginas = list_create();

	for(int i = 0;i < cantidadFramesTeoricos;i++)
	{
		t_pagina * unaPagina = malloc(sizeof(*unaPagina));

		unaPagina->nroPagina = i;
		unaPagina->nroMarco = asignarMarcoLibre();
		unaPagina->nroPaginaSwap = -1;
		unaPagina->uso = 1;
		unaPagina->modificada = 0;

		list_add(listaPaginas,unaPagina);

		pthread_mutex_lock(&mutex_algoritmo_reemplazo);
		list_add(listaPaginasClockModificado,unaPagina);
		pthread_mutex_unlock(&mutex_algoritmo_reemplazo);
	}
	return listaPaginas;
}

t_segmento * crearSegmentoSinMemoria(char * path,t_list * listaPaginas,int idSegmento,uint32_t posicionInicial,int cantidadFramesTeoricos,bool tiene_flag_shared)
{
	t_segmento * segmentoNuevo = malloc(sizeof(*segmentoNuevo));

	segmentoNuevo->esCompartido=true;
	segmentoNuevo->id_segmento=idSegmento;
	segmentoNuevo->posicionInicial = posicionInicial;
	segmentoNuevo->tamanio = cantidadFramesTeoricos*tamPagina;
	segmentoNuevo->paginas = listaPaginas;
	segmentoNuevo->archivo = strdup(path);
	segmentoNuevo->tiene_flag_shared = tiene_flag_shared;

	return segmentoNuevo;
}

int copiarDatosEnArchivo(char * path, int tamanio, void * buffer, int offset)
{
	/*
			FILE * fd = fopen(path,"a+");
				if(!fd)
					return -1;

			struct stat statbuf = {0};
			stat(path,&statbuf);

			int fd_num = fileno(fd);

			int tamanioArchivo = statbuf.st_size;

			void * bufferAuxiliar = mmap(NULL,tamanioArchivo,PROT_READ|PROT_WRITE,MAP_SHARED,fd_num,0);

			memcpy(bufferAuxiliar + offset,buffer,tamanio);

			msync(bufferAuxiliar,tamanioArchivo,MS_SYNC);

			munmap(bufferAuxiliar,tamanio);

			close(fd_num);

			return 0;
	*/

		FILE* fd = fopen(path, "r+");
		if(!fd)
			return -1;

		//struct stat statbuf = {0};
		//stat(RUTA_ARCHIVO,&statbuf);
		//int tamanioArchivo = statbuf.st_size;

		fseek(fd, offset, SEEK_SET);

		fwrite(buffer, 1, tamanio, fd);

		fclose(fd);

		return 0;


}

void liberarConUnmap(char * idProceso, t_segmento * unSegmento)
{

	char msj[450];
	char aux[100];

	if(list_size(unSegmento->paginas)==1) {
		sprintf(msj, "Para el proceso %s, se ha liberado la página ", idProceso);
	} else {
		sprintf(msj, "Para el proceso %s, se han liberado las páginas [ ", idProceso);
	}

	void liberarPaginasLocal(t_pagina* pagina) {
			sprintf(aux, "%d ",pagina->nroPagina);
			strcat(msj, aux);
			liberarPagina(pagina);;
		}
	list_destroy_and_destroy_elements(unSegmento->paginas, (void*)liberarPaginasLocal);

	strcat(msj, "]");

	unSegmento->paginas=NULL;

	loggearInfo(msj);
}

void reducirArchivoCompartido(char * idProceso, t_segmento * unSegmento)
{
	char msj[450];
	char aux[100];

	char * path = unSegmento->archivo;

	bool seEliminarPaginas = false;

	if(list_size(unSegmento->paginas)==1) {
		sprintf(msj, "Para el proceso %s, se ha liberado la página ", idProceso);
	} else {
		sprintf(msj, "Para el proceso %s, se han liberado las páginas [ ", idProceso);
	}

	bool condicion(t_archivo_compartido * unArchivoCompartido)
	{
		if(strcmp(unArchivoCompartido->nombreArchivo,path) == 0)
		{
			if(unArchivoCompartido->nroParticipantes == 1)
			{
				seEliminarPaginas = true;
				return true;
			}
			else
				unArchivoCompartido->nroParticipantes--;

		}
			return false;
	}

	void destructor(t_archivo_compartido * unArchivoCompartido)
	{
		void liberarPaginasLocal(t_pagina* unaPagina) {
			sprintf(aux, "%d ",unaPagina->nroPagina);
			strcat(msj, aux);
			liberarPagina(unaPagina);
		}
		list_destroy_and_destroy_elements(unArchivoCompartido->listaPaginas, (void*)liberarPaginasLocal);

		free(unArchivoCompartido->nombreArchivo);
		free(unArchivoCompartido);
	}

	pthread_mutex_lock(&mutex_lista_archivos);
	list_remove_and_destroy_by_condition(listaArchivosCompartidos,(void*)condicion,(void*)destructor); // ver
	pthread_mutex_unlock(&mutex_lista_archivos);

	strcat(msj, "]");

	unSegmento->paginas=NULL;

	if(seEliminarPaginas)
	loggearInfo(msj);

}

int obtenerCantidadParticipantes(char * path)
{
	t_archivo_compartido * unArchivoCompartido = obtenerArchivoCompartido(path);

	if(!unArchivoCompartido)
		return -1;

	return unArchivoCompartido->nroParticipantes;
}

t_segmento * crearSegmentoCompartido(char * idProceso,char * path, int tamanio, int cantidadFrames, bool tiene_flag_shared)
{
			t_list * segmentos;
			t_segmento* segmento;

			if(!poseeSegmentos(idProceso)) // => Es el primer malloc
			{
				segmentos = list_create();
				segmento = crearSegmentoSinMemoria(path, NULL, 0, 0, cantidadFrames,tiene_flag_shared);

				list_add(segmentos,segmento);

				pthread_mutex_lock(&mutex_diccionario);
				dictionary_remove(diccionarioProcesos, idProceso);
				dictionary_put(diccionarioProcesos, idProceso, segmentos);
				pthread_mutex_unlock(&mutex_diccionario);
			}
			else
			{
				t_segmento * ultimoSegmento;
				int nroSegmento;
				int posicionIncialSegmento;

				pthread_mutex_lock(&mutex_diccionario);
				segmentos = dictionary_get(diccionarioProcesos, idProceso);
				pthread_mutex_unlock(&mutex_diccionario);

				// mutex para las paginas del segmento
				ultimoSegmento = list_get(segmentos, list_size(segmentos) - 1); // obtengo el ultimo elemento para saber la posicion incial de este segmento

				posicionIncialSegmento = ultimoSegmento->posicionInicial + ultimoSegmento->tamanio;
				nroSegmento = ultimoSegmento->id_segmento + 1;

				segmento = crearSegmentoSinMemoria(path, NULL, nroSegmento, posicionIncialSegmento, cantidadFrames,tiene_flag_shared);

				list_add(segmentos,segmento);
			}
			return segmento;
}

char * obtenerFlag(int flag)
{
	if(flag == MUSE_MAP_SHARED)
		return "MUSE MAP SHARED";
	else
		return "MUSE MAP PRIVATE";
}


t_list * obtenerPaginasModificadasLocal(t_list * paginas)
{
	bool condicion(t_pagina * unaPagina)
	{
		return unaPagina->modificada;
	}
	return list_filter(paginas,(void*)condicion);
}


int actualizarArchivo(char * path,t_segmento * unSegmento,int posicionRelativaSegmento ,int tamanio, t_list * listaPaginasModificadas)
{

	int tamanioMaximo = (unSegmento->tamanio + unSegmento->posicionInicial) - posicionRelativaSegmento; // como lo sabria??

		if(tamanio>tamanioMaximo)
			return TAMANIO_SOBREPASADO;

	FILE * fd = fopen(path,"r+");
			if(!fd)
				return ARCHIVO_NO_EXISTENTE;
		int fd_num = fileno(fd);
	ftruncate(fd_num,unSegmento->tamanio); // lo que habia volo!

	posicionRelativaSegmento -= unSegmento->posicionInicial;

	//int bytesActualizados;
	int nroPaginaActual = (int) posicionRelativaSegmento / tamPagina;
	int bytesLeidos = 0;
	int offset = 0;
	int bytesRestantesPagina;

	if(tamanio > tamPagina)
		bytesRestantesPagina = tamPagina - posicionRelativaSegmento%tamPagina;
	else
		bytesRestantesPagina = tamanio;

	void bajarAMemoria(t_pagina * pagina)
	{
		if(bytesLeidos < tamanio)
		{
		void * buffer = malloc(tamPagina);
		if(bytesLeidos == 0)
			offset = pagina->nroMarco*tamPagina + posicionRelativaSegmento%tamPagina;
		else
			offset = pagina->nroMarco*tamPagina;

		pthread_mutex_lock(&mutex_memoria);
		if(pagina->nroPaginaSwap!=-1)
		{
			rutinaReemplazoPaginasSwap(&pagina);
		}
		offset = pagina->nroMarco*tamPagina + (offset)%tamPagina; // sumo base mas offset
		memcpy(buffer,memoria+offset,bytesRestantesPagina);
		pthread_mutex_unlock(&mutex_memoria);

		copiarDatosEnArchivo(path, bytesRestantesPagina, buffer, pagina->nroPagina*tamPagina);

		free(buffer);

		bytesLeidos += bytesRestantesPagina;

		if(tamanio - bytesLeidos < tamPagina)
			bytesRestantesPagina = tamanio - bytesLeidos;
		else
			bytesRestantesPagina = tamPagina;

		}

	}

	list_iterate(listaPaginasModificadas,(void*)bajarAMemoria);

	return nroPaginaActual;
}

void descargarAMemoria(char * archivo,t_list * listaPaginas, uint32_t posicionRelativaSegmento, int tamanio)
{
		int bytesLeidos = 0;
		int offset = 0;
		int bytesRestantesPagina;
		void * buffer = obtenerDatosArchivo(archivo, tamanio);

		if(tamanio > tamPagina)
				bytesRestantesPagina = tamPagina - posicionRelativaSegmento%tamPagina;
			else
				bytesRestantesPagina = tamanio;

		void bajarAMemoria(t_pagina * pagina)
			{
				if(bytesLeidos == 0)
					offset = pagina->nroMarco*tamPagina + posicionRelativaSegmento%tamPagina;
				else
					offset = pagina->nroMarco*tamPagina;

				pthread_mutex_lock(&mutex_memoria);
				if(pagina->nroPaginaSwap!=-1)
				{
					rutinaReemplazoPaginasSwap(&pagina);
				}
				offset = pagina->nroMarco*tamPagina + (offset)%tamPagina; // sumo base mas offset
				memcpy(memoria+offset,buffer + bytesLeidos,bytesRestantesPagina);
				pthread_mutex_unlock(&mutex_memoria);

				bytesLeidos += bytesRestantesPagina;


				if(tamanio - bytesLeidos < tamPagina)
					bytesRestantesPagina = tamanio - bytesLeidos;
				else
					bytesRestantesPagina = tamPagina;
			}

		list_iterate(listaPaginas,(void*)bajarAMemoria);

		free(buffer);
}

/*
 * uint32_t agregarPaginasSinMemoria(char * path, char * idProceso,t_archivo_compartido * unArchivoCompartido,int cantidadFramesTeoricos)
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
 */


#endif /* MUSEMEMORIACOMPARTIDA_H_ */
