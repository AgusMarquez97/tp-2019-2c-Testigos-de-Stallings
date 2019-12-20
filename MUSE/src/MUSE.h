#ifndef MUSE_H_
#define MUSE_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>


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
	MEMORIA_COMPLETA = -4,
	TAMANIO_ARCHIVO_SOBREPASADO = -5,
	PAGINA_NO_VALIDA = -6
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
pthread_mutex_t mutex_marcos_swap_libres;
pthread_mutex_t mutex_diccionario;
pthread_mutex_t mutex_lista_archivos;

pthread_mutex_t mutex_algoritmo_reemplazo;


pthread_mutex_t mutex_lista_paginas;
pthread_mutex_t mutex_segmento;

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
t_list * listaArchivosCompartidos;
t_list * listaPaginasClockModificado;

int ptrAlgoritmoPaginaSiguiente;

typedef struct {
	int id_segmento;
	uint32_t posicionInicial;
	int tamanio; //tamanio que sera multiplo del tam de pag siendo este el menor tamanio posible
	bool esCompartido;
	t_list * paginas;
	char * archivo;
	bool tiene_flag_shared;
} t_segmento;

typedef struct {
	int nroPagina;
	int nroPaginaSwap;
	int nroMarco;// permite calcular la direccion inicial en MP;
	bool uso;
	bool modificada;
} t_pagina;

typedef struct {
	char * nombreArchivo;
	int nroParticipantes;
	t_list * listaPaginas;
} t_archivo_compartido;


/*
 * Muse
 */

int main();
void levantarConfig();
void levantarMemoria();
void levantarMarcos(t_bitarray** unBitArray, int tamanio, int* cantidadMarcos);
void crearMemoriaSwap();
void inicializarSemaforos();
void levantarServidorMUSE();
void rutinaServidor(int* socketRespuesta);
void liberarVariablesGlobales();
void salirFuncion(int pid);

/*
 * Muse Operaciones
 */
int procesarHandshake(char* idProceso);
uint32_t procesarMalloc(char* idProceso, int32_t tamanio);
int32_t procesarFree(char* idProceso, uint32_t posicionSegmento);
int procesarCpy(char* idProceso, uint32_t posicionSegmento, int32_t tamanio, void* contenido);
void* procesarGet(char* idProceso, uint32_t posicionSegmento, int32_t tamanio);
uint32_t procesarMap(char* idProceso, char* path, int32_t tamanio, int32_t flag);
int procesarSync(char* idProceso, uint32_t posicionSegmento, int32_t tamanio);
int procesarUnmap(char* idProceso, uint32_t posicionSegmento);
int procesarClose(char* idProceso);

/*
 * Muse Malloc
 */
uint32_t analizarSegmento (char* idProceso, int tamanio, int cantidadFrames, bool esCompartido);
void crearSegmento(char* idProceso, int tamanio, int cantidadMarcos, t_list* listaSegmentos, int idSegmento, bool esCompartido, int posicionInicial);
t_segmento* instanciarSegmento(int tamanio, int cantidadFrames, int idSegmento, bool esCompartido, int posicionInicial);
t_list* crearListaPaginas(int tamanio, int cantidadMarcos,bool esCompartido);
void agregarPaginas(t_list** listaPaginas, int cantidadMarcos, int nroUltimaPagina, bool esCompartido);
uint32_t completarSegmento(char * idProceso,t_segmento* segmento, int tamanio);
int estirarSegmento(int baseSegmento,char* idProceso, t_segmento* segmento, int tamanio, int nuevaCantidadFrames, int offset, int sobrante, int paginaActual);
int asignarMarcoLibre();

/*
 * Muse Free
 */
int analizarFree(char* idProceso, uint32_t posicionSegmento);
int liberarUnHeapMetadata(t_list * paginas, int offsetSegmento);
int defragmentarSegmento(t_segmento* segmento);
void compactarSegmento(char* idProceso, t_segmento* segmento);
void liberarPaginas(char* idProceso, int nroPagina, t_segmento* segmento);
void liberarPagina(t_pagina* pagina);

/*
 * Muse Cpy/Get
 */
void * analizarGet(char* idProceso, uint32_t posicionSegmento, int32_t tamanio);
int leerUnHeapMetadata(t_list * paginas,int posicionSegmento, void ** buffer, int tamanio);
void leerDatosMemoria(t_list * paginas,int paginaActual, int posicionMemoria, void ** buffer, int tamanio);

int analizarCpy(char* idProceso, uint32_t posicionSegmento, int32_t tamanio, void* contenido);
int escribirDatosHeapMetadata(t_list * paginas, int posicionSegmento, void ** buffer, int tamanio);
void escribirDatosHeap(t_list * paginas,int paginaActual, int posicionPosteriorHeap, void ** buffer, int tamanio);

t_heap_metadata * recuperarHeapMetadata(t_list * listaPaginas, uint32_t cantidadBytes, int * cantidadBytesRestantes);

int analizarGetMemoriaMappeada(char* idProceso,t_segmento * unSegmento, uint32_t posicionRelativaSegmento, int32_t tamanio, void ** buffer);
int analizarCpyMemoriaMappeada(char* idProceso,t_segmento * unSegmento, uint32_t posicionRelativaSegmento, int32_t tamanio, void ** contenidoACopiar);

/*
 * Muse Memoria Compartida
 */
uint32_t analizarMap(char* idProceso, char* path, int32_t tamanio, int32_t flag);
int analizarSync(char* idProceso, uint32_t posicionSegmento, int32_t tamanio);
int analizarUnmap(char* idProceso, uint32_t posicionSegmento);

void * obtenerDatosArchivo(char * path, int tamanio);
t_archivo_compartido * agregarArchivoLista(char * unArchivo, t_archivo_compartido * archivoCompartido, t_list * listaPaginas);
t_archivo_compartido * obtenerArchivoCompartido(char * path);
//uint32_t agregarPaginasSinMemoria(char * path, char * idProceso,t_archivo_compartido * unArchivoCompartido,int cantidadFramesTeoricos);
t_list * crearPaginasSinMemoria(int cantidadFramesTeoricos);
t_segmento * crearSegmentoSinMemoria(char * path,t_list * listaPaginas,int idSegmento,uint32_t posicionInicial,int cantidadFramesTeoricos,bool tiene_flag_shared);
int copiarDatosEnArchivo(char * path, int tamanio, void * buffer, int offset);
void liberarConUnmap(char * idProceso, t_segmento * unSegmento);
void reducirArchivoCompartido(char * idProceso, t_segmento * unSegmento);
int obtenerCantidadParticipantes(char * path);
int actualizarArchivo(char * path,t_segmento * unSegmento,int posicionRelativaSegmento ,int tamanio, t_list * listaPaginasModificadas);
t_list * obtenerPaginasModificadasLocal(t_list * paginas);
/*
 * Crea un segmento compartido y lo añade a la lista
 */
t_segmento * crearSegmentoCompartido(char * idProceso,char * path, int tamanio, int cantidadFrames, bool tiene_flag_shared);
char * obtenerFlag(int flag);

/*
 * Muse Memoria Swap
 */
void moverMarcosASwap();
void rutinaReemplazoPaginasSwap(t_pagina** unaPagina);
t_pagina * ejecutarAlgoritmoReemplazo();
void reemplazarVictima(t_pagina ** paginaVictima, bool bloqueoMarco);
void recuperarPaginaSwap(t_pagina ** paginaActualmenteEnSwap,int marcoObjetivo);
void escribirSwap(int nroPagina, void * buffer);
void * leerSwap(int nroPagina);
bool estaEnMemoria(t_list * paginas, int nroPagina);
int asignarMarcoLibreSwap();
bool estaLibreMarcoMemoriaSwap(int nroMarco);
void* leerDeMemoria(int posicionInicial, int tamanio);
void escribirEnMemoria(void* contenido, int posicionInicial, int tamanio);
void eliminarDeAlgoritmo(t_pagina * unaPagina);

/*
 * Muse Heap Metadata
 */
void leerHeapMetadata(t_heap_metadata** heapMetadata, int* bytesLeidos, int* bytesLeidosPagina, int* offset, t_list * paginas, int* nroPagina);
void leerHeapPartido(t_heap_metadata** heapMetadata, int* offset, int sobrante, int* nroPagina, t_list* paginas, t_pagina** paginaDummy);

int escribirUnHeapMetadata(t_list * listaPaginas,int paginaActual,t_heap_metadata * unHeapMetadata, int * offset, int tamanioPaginaRestante);
int escribirHeapMetadata(t_list * listaPaginas,int paginaActual, int offset, int tamanio, int offsetMaximo);

t_heap_metadata * obtenerHeapMetadata(t_list * listaPaginas, int offsetPrevioHM, int nroPagina);
bool existeHM(t_list * paginas, int offsetBuscado);

/*
 * Muse Auxiliares
 */
bool existeEnElDiccionario(char* idProceso);
int obtenerCantidadMarcos(int tamanioPagina, int tamanioMemoria);
t_segmento* obtenerUnSegmento(char * idProceso, uint32_t posicionMemoria);
t_segmento* obtenerSegmento(t_list* segmentos, uint32_t posicionMemoria);
t_pagina* obtenerPagina(t_list* paginas, uint32_t posicionSegmento);
bool poseeSegmentos(char* idProceso);
int cantidadPaginasPedidas(int offset);
void liberarMarcoBitarray(int nroMarco);
void liberarPaginasSwap(int nroPaginaSwap);
bool estaLibreMarco(int nroMarco);
bool existeOtraPaginaConElMarco(t_list * listaPaginas,int nroMarco, int paginaActual);
uint32_t obtenerDireccionMemoria(t_list* listaPaginas,uint32_t posicionSegmento);
t_segmento * buscarSegmento(t_list * segmentos, uint32_t posicionSegmento);
t_pagina * obtenerPaginaAuxiliar(t_list * paginas, int nroPagina);
t_list * obtenerPaginas(char* idProceso, uint32_t posicionSegmento);
void usarPagina(t_list * paginas, int nroPagina);
int obtenerNroPagina(t_list * paginas, int offsetSegmento);
int obtenerOffsetPrevio(t_list * paginas, int offsetSegmento, int nroPaginaActual);
int obtenerOffsetPosterior(t_list * paginas, uint32_t posicionSegmento,int nroPaginaActual);


#endif /* MUSE_H_ */
