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



/*
 * 	SUSE
 */

typedef enum {
	HANDSHAKE_SUSE,
	CREATE,
	NEXT,
	JOIN,
	RETURN,

} t_operacionSuse;

typedef struct {
	int32_t idProceso;
	int32_t tipoOperacion;
	int32_t idHilo;
	int32_t rafaga;
} t_mensajeSuse;

typedef struct {
	int32_t idProceso;
	int32_t idHilo;
	int32_t rafaga;
} t_hiloPlanificado;

#endif /* ENUMSANDSTRUCTS_H_ */
