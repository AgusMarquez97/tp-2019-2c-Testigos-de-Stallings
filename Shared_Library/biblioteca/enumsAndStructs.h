/*
 * enumsAndStructs.h
 *
 *  Created on: Sep 8, 2019
 *      Author: agus
 */

#ifndef ENUMSANDSTRUCTS_H_
#define ENUMSANDSTRUCTS_H_

#include <stdint.h>

/*
 * MUSE
 */

typedef enum {

	HANDSHAKE = 1, MALLOC = 2,FREE = 3 , GET = 4, CPY = 5, MAP = 6, SYNC = 7, UNMAP = 8 ,CLOSE = 9

} tipoOperacionMuse;

typedef enum {

	MUSE_MAP_PRIVATE = 1,  MUSE_MAP_SHARED = 2

} flagsMuse;

typedef struct {

	int32_t tipoOperacion;
	int32_t idProceso;

	int32_t tamanioVariable;

	uint32_t posicionMemoria; // Free, Malloc, etc

	int32_t tamanio;
	void * contenido;

	uint32_t dst;

	int32_t flag;
}mensajeMuse;

// IMPORTANTE: DEFINIR QUE VAMOS A ENVIAR Y RECIBIR -> SE VAN A COMUNICAR ENTRE SI SUSE-MUSE-FUSE -> NO




#endif /* ENUMSANDSTRUCTS_H_ */
