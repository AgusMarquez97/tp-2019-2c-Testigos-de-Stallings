#ifndef ENUMSANDSTRUCTS_H_
#define ENUMSANDSTRUCTS_H_

#include <stdint.h>

/*
 * 	MUSE
 */

typedef enum {
	HANDSHAKE,
	MALLOC,
	FREE,
	GET,
	CPY,
	MAP,
	SYNC,
	UNMAP,
	CLOSE
} t_operacionMuse;

typedef enum {
	MUSE_MAP_PRIVATE,
	MUSE_MAP_SHARED
} t_flagMuse;

typedef struct {
	int32_t idProceso;
	int32_t tipoOperacion;
	uint32_t posicionMemoria;
	int32_t tamanio;
	void* origen;
	char* contenido;
	int32_t flag;
} t_mensajeMuse;

#endif /* ENUMSANDSTRUCTS_H_ */
