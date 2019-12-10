#ifndef MUSE_H_
#define MUSE_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

#include <biblioteca/sockets.h>
#include <biblioteca/serializacion.h>
#include <biblioteca/mensajes.h>
#include <biblioteca/enumsAndStructs.h>
#include <biblioteca/logs.h>
#include <biblioteca/levantarConfig.h>
#include <biblioteca/utils.h>
#include <biblioteca/mensajesMuse.h>

#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/node.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>
#include <commons/bitarray.h>
#include <commons/txt.h>

#define pathConfig "/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/MUSE/config/configuracion.txt"
#define tam_heap_metadata sizeof(t_heap_metadata)

typedef enum {
	HM_NO_EXISTENTE = -1,
	TAMANIO_SOBREPASADO = - 2, // = SEG FAULT
	HM_YA_LIBERADO = -3,
	MEMORIA_COMPLETA = -4
} t_errores;

char ip[46];
char puerto[10];

/*
 * Semaforos:
 * mutex_marcos_libres : Mutex para que dos hilos no pidan el mismo marco al mismo tiempo
 * mutex_diccionario: No quiero que al modificar los datos del diccionario (debo sacar y agregar el mismo elemento) alguien consulte por este
 */

pthread_mutex_t mutex_memoria;
pthread_mutex_t mutex_marcos_libres;
pthread_mutex_t mutex_diccionario;

/*
 * Estructura de la memoria principal:
 * 1° Tamanio de la memoria principal
 * 2° Bloque contiguo de datos
 * 3° Un bitarray para identificar huecos libres
 * 4° Un tamanio de marcos que dividiran logicamente a la memoria
 * 5° Una estructura que determina como se escribe la info. En este caso => t_heap_metadata
 */

int tamMemoria;
char* memoria;
t_bitarray* marcosMemoriaPrincipal;
int tamPagina;
int cantidadMarcosMemoriaPrincipal;

typedef struct __attribute__((packed)) {
	uint32_t offset;
	bool estaLibre;
} t_heap_metadata;


/*
* diccionario => nombre_archivo
* 									key
* 										-> lista de marcos
* 										-> contador de procesos
 */

/*
 * Estructura de la memoria swap:
 * 1° Un tamanio de la memoria swap
 * 2° Path del archivo a crear
 * 3° Un bitarray para identificar huecos libres
 * 4° Tamanio de marcos (= memoria principal)
 * 5° Misma estructura t_heap_metadata
 */

int tamSwap;
char* path_archivo_swap;
t_bitarray * marcosMemoriaSwap;
int cantidadMarcosMemoriaVirtual;

/*
 * Estructuras para acceder y administrar la memoria principal y la memoria swap:
 * 1° Un diccionario de procesos y archivos, donde cada proceso/archivo posee una lista de segmentos y cada segmento una lista de paginas
 * Nota: En el caso de los archivos hay que tenes mas consideraciones => otro tipo de segmentos
 * Pendiente: Agregar lo necesario para las metricas del algoritmo de reemplazo
 * Importante: Un segmento no podra ser prolongado, unicamente si existe otro segmento de mmap. El segmento da una idea de memoria contigua, por esto tiene
 * un punto de referencia y un tamanio/offset
*/

t_dictionary * diccionarioProcesos;

typedef struct {
	int id_segmento;
	uint32_t posicionInicial;
	int tamanio; //tamanio que sera multiplo del tam de pag siendo este el menor tamanio posible
	bool esCompartido;
	t_list * paginas;
} t_segmento;

typedef struct {
	int nroPagina;
	int nroMarco;// permite calcular la direccion inicial en MP;
	//bit presencia
	//bit modificado
} t_pagina;

// MUSE
int main();
void levantarConfig();
void levantarMemoria();
void levantarMarcos(t_bitarray** unBitArray, int tamanio, int* cantidadMarcos);
void crearMemoriaSwap();
void inicializarSemaforos();
void levantarServidorMUSE();
void rutinaServidor(int* socketRespuesta);
void liberarVariablesGlobales();

// MuseOperaciones
bool existeEnElDiccionario(char* idProceso);
int procesarHandshake(char* idProceso);
uint32_t procesarMalloc(char* idProceso, int32_t tamanio);
uint32_t obtenerDireccionMemoria(t_list* listaPaginas,uint32_t posicionSegmento);
int defragmentarSegmento(t_segmento* segmento);
void compactarSegmento(char* idProceso, t_segmento* segmento);
int32_t procesarFree(char* idProceso, uint32_t posicionSegmento);
void* procesarGet(char* idProceso, uint32_t posicionSegmento, int32_t tamanio);
int procesarCpy(char* idProceso, uint32_t posicionSegmento, int32_t tamanio, void* contenido);
uint32_t procesarMap(char* idProceso, char* path, int32_t tamanio, int32_t flag);
int procesarSync(char* idProceso, uint32_t posicionMemoria, int32_t tamanio);
int procesarUnmap(char* idProceso, uint32_t posicionMemoria);
int procesarClose(char* idProceso);
uint32_t analizarSegmento (char* idProceso, int tamanio, int cantidadFrames, bool esCompartido);

// MuseAuxiliares
int obtenerCantidadMarcos(int tamanioPagina, int tamanioMemoria);
t_segmento* obtenerSegmento(t_list* segmentos, uint32_t posicionMemoria);
t_pagina* obtenerPagina(t_list* paginas, uint32_t posicionMemoria);
bool paginaCorrespondiente(t_pagina* pagina);
bool poseeSegmentos(char* idProceso);
void agregarPaginas(t_list** listaPaginas, int cantidadMarcos, int nroUltimaPagina);
t_list* crearListaPaginas(int tamanio, int cantidadMarcos);
t_segmento* instanciarSegmento(int tamanio, int cantidadFrames, int idSegmento, bool esCompartido, int posicionInicial);
void crearSegmento(char* idProceso, int tamanio, int cantidadFrames, t_list* listaSegmentos, int idSegmento, bool esCompartido, int posicionInicial);
uint32_t completarSegmento(char* idProceso, t_segmento* ultimoSegmento, int tamanio);
void estirarSegmento(char * idProceso,t_segmento * segmento,int tamanio,int nuevaCantidadFrames,int offset, int sobrante);
int cantidadPaginasPedidas(int offset);
void* leerDeMemoria(int posicionInicial, int tamanio);
void escribirEnMemoria(void* contenido, int posicionInicial, int tamanio);
void liberarMarcoBitarray(int nroMarco);
bool estaLibreMarco(int nroMarco);
int asignarMarcoLibre();
int obtenerPaginaActual(t_list * paginas, int offset);
uint32_t obtenerDireccionMemoria(t_list* listaPaginas,uint32_t posicionSegmento);
t_segmento * buscarSegmento(t_list * segmentos, uint32_t posicionSegmento);
bool encontrarSegmento(t_segmento * unSegmento);
void liberarPagina(int nroPagina, t_list* paginas);
void liberarPaginas(char* idProceso, int nroPagina, t_segmento* segmento);
t_list * obtenerPaginas(char* idProceso, uint32_t posicionSegmento);

//MuseHeapMetadata
int leerUnHeapMetadata(t_list * paginas, int posicionAnteriorHeap,int posicionPosteriorHeap, void ** buffer, int tamanio);
int liberarUnHeapMetadata(t_list * paginas, int offset);
void leerDatosHeap(t_list * paginas, int offset, void ** buffer, int tamanio);
void leerHeapMetadata(t_heap_metadata** heapMetadata, int* bytesLeidos, int* bytesLeidosPagina, int* offset, t_list * paginas, int* nroPagina);
void leerHeapPartido(t_heap_metadata** heapMetadata, int* offset, int sobrante, int* nroPagina, t_list* paginas, t_pagina** paginaDummy);
int escribirHeapMetadata(t_list * listaPaginas, int offset, int tamanio, int offsetMaximo);
int escribirDatosHeapMetadata(t_list * paginas, int posicionAnteriorHeap,int posicionPosteriorHeap, void ** buffer, int tamanio);
void escribirDatosHeap(t_list * paginas, int posicionPosteriorHeap, void ** buffer, int tamanio);
t_heap_metadata * obtenerHeapMetadata(t_list * listaPaginas, int offset);
uint32_t obtenerPosicionPreviaHeap(t_list * paginas, int offset);
t_pagina * obtenerPaginaAuxiliar(t_list * paginas, int nroPagina);
bool existeHM(t_list * paginas, int offsetBuscado);
int escribirUnHeapMetadata(t_list * listaPaginas,int paginaActual,t_heap_metadata * unHeapMetadata, int * offset, int tamanioPaginaRestante);

#endif /* MUSE_H_ */
