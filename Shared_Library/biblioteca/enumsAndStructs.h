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
	void* contenido;
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

typedef enum {
	NEW,
	READY,
	EXEC,
	BLOCK,
	EXIT,

} t_estadoHilo;

typedef struct {
	int32_t idProceso;
	int32_t tipoOperacion;
	int32_t idHilo;
	int32_t rafaga;
} t_mensajeSuse;

typedef struct {
	char * idProceso; // char * para meterlo en diccionario
	int32_t idHilo;
	int32_t estadoHilo;
	int32_t rafaga;
} t_hiloPlanificado;


/*
 * 	FUSE
 */

typedef enum {
	HANDSHAKE_FUSE,
	GETATTR,
	READDIR,
	READ,
	MKDIR,
	MKNOD,
	WRITE,
	RMDIR,
	UNLINK,
	RENAME,
} t_operacionFuse;


typedef struct {
	int32_t tipoOperacion;
	char* path;
	int32_t idHilo;
	int32_t rafaga;
} t_mensajeFuse;


#endif /* ENUMSANDSTRUCTS_H_ */
