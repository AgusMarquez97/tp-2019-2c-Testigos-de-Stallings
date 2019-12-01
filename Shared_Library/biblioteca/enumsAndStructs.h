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
	MUSE_MAP_PRIVATE = 1,
	MUSE_MAP_SHARED = 2
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
	CREATE,
	NEXT,
	JOIN,
	CLOSE_SUSE,
	WAIT,
	SIGNAL

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
	char * semId;//semaforo que nos mandan para hacerle wait o signal
} t_mensajeSuse;

typedef struct {
	char * idProceso; // char * para meterlo en diccionario
	int32_t idHilo;
	int32_t estadoHilo;
	int32_t rafaga;
	char * semBloqueante;
	int32_t hiloBloqueante;

	int32_t timestampCreacion; //time_t
	int32_t timestampEntraEnReady;
	int32_t tiempoEnReady;
	int32_t tiempoEnExec;
	int32_t timestampEntra;
	int32_t timestampSale;
	int32_t estimado;


} t_hiloPlanificado;




typedef struct {
	char * idSem;
	int32_t valorActual;
	int32_t valorMax;
} t_semaforoSuse;


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
	UTIMENS,
	TRUNCATE,
} t_operacionFuse;


typedef struct {
	int32_t tipoOperacion;
	char* path;
	int32_t idHilo;
	int32_t rafaga;
} t_mensajeFuse;

typedef enum {
	BORRADO,
	ARCHIVO,
	DIRECTORIO,
} t_estadoNodo;

#endif /* ENUMSANDSTRUCTS_H_ */
