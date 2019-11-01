/*
 * MuseOperaciones.h
 *
 *  Created on: Oct 31, 2019
 *      Author: agus
 */

#ifndef MUSEOPERACIONES_H_
#define MUSEOPERACIONES_H_

#include "MuseAuxiliares.h"

int agregarADiccionario(int idProceso)
{
	char id_p[30];
	char msj[120];
	sprintf(id_p,"%d",idProceso);

	if(!dictionary_is_empty(dictionarioProcesos))
	{
		if(dictionary_has_key(dictionarioProcesos,id_p))
		{
			sprintf(msj,"Fallo al realizar el handshake, init duplicado del proceso %s", id_p);
			loggearInfo(msj);
			return -1;
		}
	}

	dictionary_put(dictionarioProcesos,id_p,NULL);
	sprintf(msj,"Handshake exitoso. Proceso %s agregado al diccionario de procesos correctamente",id_p);
	loggearInfo(msj);

	return 1;
}

uint32_t procesarMalloc(int32_t idProceso, int32_t tamanio) {
	return 1;
}

uint32_t procesarFree(int32_t idProceso, uint32_t posicionMemoria) {
	return 1;
}

char* procesarGet(int32_t idProceso, uint32_t posicionMemoria, int32_t tamanio) {
	return "PRUEBA";
}

int procesarCpy(int32_t idProceso, uint32_t posicionMemoria, int32_t tamanio, void* origen) {

	char* cadena = malloc(tamanio + 1);

	memcpy(cadena, origen, tamanio);
	cadena[tamanio] = 0;
	loggearInfo(cadena);
	free(cadena);

	return 0;

}

uint32_t procesarMap(int32_t idProceso, void* contenido, int32_t tamanio, int32_t flag) {

	char* pathArchivo = malloc(tamanio + 1);

	memcpy(pathArchivo, contenido, tamanio);
	pathArchivo[tamanio] = 0;
	loggearInfo(pathArchivo);
	free(pathArchivo);

	return 1;

}

int procesarSync(int32_t idProceso, uint32_t posicionMemoria, int32_t tamanio) {
	return 0;
}

int procesarUnmap(int32_t idProceso, uint32_t posicionMemoria) {
	return 0;
}

int procesarClose(int32_t idProceso) {
	char id_p[30];
	char msj[120];
	sprintf(id_p,"%d",idProceso);

	t_list * segmentos;

	segmentos = dictionary_get(dictionarioProcesos,id_p);

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

	dictionary_remove(dictionarioProcesos,id_p);

	sprintf(msj,"Proceso %s eliminado del diccionario de procesos",id_p);
	loggearInfo(msj);

	return 0;
}



#endif /* MUSEOPERACIONES_H_ */
