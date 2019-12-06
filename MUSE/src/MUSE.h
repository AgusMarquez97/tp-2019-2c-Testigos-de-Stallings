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
void defragmentarSegmento(t_segmento* segmento);
int32_t procesarFree(char* idProceso, uint32_t posicionSegmento);
void* procesarGet(char* idProceso, uint32_t posicionSegmento, int32_t tamanio);
int procesarCpy(char* idProceso, uint32_t posicionSegmento, int32_t tamanio, void* contenido);
uint32_t procesarMap(char* idProceso, void* contenido, int32_t tamanio, int32_t flag);
int procesarSync(char* idProceso, uint32_t posicionMemoria, int32_t tamanio);
int procesarUnmap(char* idProceso, uint32_t posicionMemoria);
int procesarClose(char* idProceso);
uint32_t analizarSegmento (char* idProceso, int tamanio, int cantidadFrames, bool esCompartido);

// MuseAuxiliares
int obtenerCantidadMarcos(int tamanioPagina, int tamanioMemoria);
t_segmento* obtenerSegmento(t_list* segmentos, uint32_t posicionMemoria);
bool segmentoCorrespondiente(t_segmento* segmento);
t_pagina* obtenerPagina(t_list* paginas, uint32_t posicionMemoria);
bool paginaCorrespondiente(t_pagina* pagina);
bool poseeSegmentos(char* idProceso);
void agregarPaginas(t_list** listaPaginas, int cantidadMarcos, int nroUltimaPagina);
t_list* obtenerPaginas(int tamanio, int cantidadMarcos);
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

//MuseHeapMetadata
int leerUnHeapMetadata(t_list * paginas, int offset, void ** buffer, int tamanio);
int liberarUnHeapMetadata(t_list * paginas, int offset);
void leerDatosHeap(t_list * paginas, int offset, void ** buffer, int tamanio);
void leerHeapMetadata(t_heap_metadata** heapMetadata, int* bytesLeidos, int* bytesLeidosPagina, int* offset, t_list * paginas, int* nroPagina);
void leerHeapPartido(t_heap_metadata** heapMetadata, int* offset, int sobrante, int* nroPagina, t_list* paginas, t_pagina** paginaDummy);
int escribirHeapMetadata(t_list * listaPaginas, int offset, int tamanio);
int escribirUnHeapMetadata(t_list * paginas, int offset, void ** buffer, int tamanio);
void escribirDatosHeap(t_list * paginas, int offset, void ** buffer, int tamanio);
t_heap_metadata * obtenerHeapMetadata(t_list * listaPaginas, int offset);
int obtenerPosicionPreviaHeap(t_list * paginas, int offset);
bool existeHM(t_list * paginas, int offsetBuscado);

#endif /* MUSE_H_ */

/*
 * Consideraciones
 * Se retornar direcciones virtuales, esto quiere decir que no retorno la direccion real de la MP, si no una abstraccion:
 * EJ:
 * Segmento A [0-100]
 * Memoria [0-1000]
 * Las direcciones de memoria que conoce el proceso son las del segmento A, si realiza un malloc de 10 y el heap_metada=5, el programa retorna la posicion 5 como primera posicion disponible,
 * cuando realmente esta posicion puede ser cualquiera de memoria.
 */

/*
 * Se dispone el siguiente ejemplo real para entender bien el funcionamiento: SE OMITE EL CASO DE MEMORIA VIRTUAL O COMPARTIDA
 * *Se supone pags = 20 y tam del t_heap_metadata = 4
 *
 * A) P1 realiza un malloc(10)
 *
 *	Parte Fisica:
 *
 * 		=> 1° Se calcula que cantidad de paginas tendra el segmento (en este caso se necesita solo 1 pag ya que tam malloc < tam de pag), al saber que
 * 		necesito una sola pagina solicito un marco libre (si no hay entra en juego el algoritmo y la MV [se evita por simplicidad]).
 * 		Para saber que marco esta libre recorro el bitmap.
 * 		* Para el ejemplo tengo el marco libre = 5 *	(en gral retornara una lista de marcos [5])
 *
 * 		Por ultimo, se actualiza el valor del bitmap para indicar que el marco 5 esta ocupado
 *
 * 		=> 2° Una vez obtenido el frame, me dispongo a ESCRIBIR en la memoria => tengo que escribir las estructuras metadatas que luego seran leidas
 *
 * 		Prosigo a calcular la lista de t_heap_metadatos (en este caso 2, 1 para lo que uso y otra para identificar lo que me quede libre)
 *
 * 		t_list * lista_metadatos = [
 * 		{
 * 			offset = 10,
 * 			isFree = false
 * 		}, // Lo que voy a usar para el malloc
 * 		{
 * 			offset = 2,
 * 			isFree = true
 * 		} // Lo que me sobro de la pagina, validar que sucede si solo me queda espacio para que entre la estructura Heap_metadata
 * 		]
 * 		Luego se recorre dicha lista aplicando en cada caso lo siguiene:
 *
 *
 * 		list_iterate(lista_metadatos,escribir_metadato);
 *
 * 		int offset = 0;
 * 		int pos_mem = 0;
 * 		int nro_frame = frame_obtenido_con_bitmap;
 *
 *
 * 		void escribir_metadato(t_heap_metadata * unMetadato) // recorre la lista
 * 		{
 * 		 pos_mem = tam_pag/frame * nro_frame + offset;
 * 		 memcpy(memoria + pos_mem,unMetadato,sizeof(lista_metadatos)); // Escribo en memoria la estructura para hacer las validaciones
 * 		 offset += sizeof(lista_metadatos) + unMetadato->offset;
 * 		}
 *
	Parte Logica/administrativa:
 * 		=> 1° MUSE agrega una entrada al dictionario de procesos
 * 		=> 2° Se prosiguen a crear las paginas necesarias con los valores de frame obtenidos: (en este caso 1 sola)
 *
 * 		t_pagina una_pagina =
 * 		{
 * 			nroPagina = 0, // La idea es ir validando, empezando desde 0 para ver si ya existe alguna pagina con ese nro. (me ahorro el bitmap)
 * 			nroMarco = 5 // el que ya obtuve antes. Luego deberia aplicar una logica para determinar el marco de la lista de marcos que obtuve
 * 			//bit presencia = 1
			//bit modificado = 0
 * 		}
 *
 * 		=> 3° Se prosigue a crear un segmento con los siguientes datos:
 * 				{
 * 					id_seg = 0, // Misma logica que para determinar el nro de pag
 * 					posicionInicial = 0hx // Posicion logica del segmento
 * 					tamanio = 20 // Extension del segmento => siempre sera multiplo del tam de pag
 * 					esCompartido = falso // Por defecto no, ya que no es segmento mmap
 * 					paginas = [0];
 * 				}
 * 		=> 4° Finalmente, se prosigue a retorna la direccion:
 *
 * 		*Comentarios:
 * 		En ningun momento se retornar las direcciones reales de las paginas/frames, siempre se retorna la direccion del segmento como una abstraccion a la memoria.
 *
 * 		Dicho esto, la direccion de retorno es = 0 (primera posicision del segmento) + 4 (el tamanio de la estructura metadata) + 0 (offset = bytes ya utilizados del segmento)
 *
 * 		*Podria haber otro proceso con la direccion 4, ya que seria un proceso 2 con segmento 0 recien creado. Esto no implica que comparta memoria con P1 ya que al momento de desrefenciar ambas direcciones tendras
 * 		distintas direcciones fisicas.
 *
 * 	B) Luego P1 desea desrefenciar dicha memoria (ya sea para hacer un cpy, get o free). Llamemos dir_deseada a dicha direccion = 4.
 *
 * 		1° Parte de indexacion:
 * 			a) Busco en el diccionario el ID del proceso y obtengo la lista de segmentos asociada.
 * 			b) Busco en el segmento 0 ya que la dir_deseada cae dentro del rango [pos_inicial-tamanio] de dicho segmento = [0-20]
 * 			c) Busco las paginas de dicho segmento, y valido en que pag cae esa dir_deseada, como la pag 0 [0-20] se que la info se encuentra en dicha pagina
 * 			d) Busco el frame asociado a dicha pagina (si dicha pagina esta en MP) => dir_inicial = nro_frame * tam_frame
 * 			e) Busco pararme luego del heap_metadata => se analizan 2 situaciones:
 *
 * 					1.e.1) Si la dir_deseada < tam_pag (cae en la pagina 0)
 * 					=> accedo a la posicion de dicho frame, 4 en este caso (sabiendo que si o si tengo una estructura metadata atras)
 * 					=> offset = 4
 	 	 	 	 	=> dir_final_memoria = dir_inicial + offset
 *
 * 					1.e.2) Si la dir deseada > tam_pag (cae en las paginas [1-n])
 * 					=> realizo la operacion: dir_deseada - tam_pag * nro_pag_obtenido y obtengo la primera direccion que esta luego del heap_metadata
 * 					=> offset = dir_deseada - tam_pag * nro_pag [nunca sera negativa]
 	 	 	 	 	=> dir_final_memoria = dir_inicial + offset

 *
 * 					EJ: si tengo toda la pag 0 ocupada y quiero acceder a la dir deseada 30 (asumiendo que ya se realizo el malloc correspondiente) y suponiendo que estoy en el frame 8,
 * 					=> para leer los datos debo realizar la operacion descripta arriba => 30 - 20*1 = 10, 10 es el desplazamiento que debo realizar en el frame 8 para leer el primer dato del heap_metada a acceder
 * 					=> Entonces, mi direccion final de memoria (la fisica) es = nro_frame*tam_pag + offset => 20*8 + 10 = 170. A partir de la dir 170 hay datos

 *
 * 			f) Obtengo los valores del heap_metadata que estan justo antes de la direccion obtenida en e), esto lo necesito para validar que el free/cpy/get sea valido
 * 			g) Una vez accedi a la primera posicion fisica donde estan los datos:
 * 			 Realizo lo que corresponda en cada caso y retorno un valor:
 * 				caso free:
 * 					Me paro en la posicion indicada y realizo un memset del offset del heap_metadata(HM) (desde donde esto parado resto sizeof(HM) y libero sizeof(HM)
 * 					Por ultimo valido si toda la pagina esta libre => actualizo el bitmap de frames
 * 				caso cpy:
					Me paro en la posicion indicada y realizo un memcpy con los datos recibidos por el tamanio pedido
				caso get:
					Me paro en la posicion indicada y realizo un memcpy a una variable char * la cantidad de bytes que se quieran leer, por ultimo retorno dicha variable
 */

/*
 * MALLOC: Pasos a seguir
 * 1° Buscar en el diccionario el proceso correspondiente
 * 2° Calcular cuantos frames necesito para asignar la memoria correspondiente ()
 * 3° Analizar si el segmento del proceso existe .Si no existe lo creo, si no evalua lo siguiente:
 *  	3A) Puedo agrandar el segmento => lo agrando, busco los frames que necesito, los escribo (considero la estructura HeapMetadata) y retorno la direccion del segmento
 *  	3B) No puedo agrandar mas el segmento por un mmap => Creo un nuevo segmento en la lista de segmentos (Por ahora no va a pasar)
 *
 */

/*
 * funcion para escribir el heap metadata en la memoria:
 * 			0 - Creo un heap metadata con el tamanio a escribir (tam del malloc) y el estado en ocupado
 * 			1 - Escribo siempre el heap metadata en la primera posicion de memoria del primerMarco
 * 			Luego calculo lo siguiente
 *
 * 				A) Cantidad de espacio reservado en memoria = cantidad_marcos_pedidos * tam_marco/pag
 * 				B) Cantidad de espacio usado realmente = tam_malloc + tam_heap_metadata
 * 				C) El tamanio que me sobro = A - B
 * 			2- Analizo si C > tam_heap_metadata
 * 					=> Verdadero: entonces creo un nuevo heap metadata en la posicion: (tamanio total - C)
 * 					=> Falso: No creo nada, asumo fragmentacion interna en la ultima pagina
 */

/*
 * 1- Obtiene el ultimo segmento
 * 2- Escribe la cantidad de paginas necesaria
 *
 * Por ahora, no valida:
 * 			Que pasa si no hay mas lugar en la memoria principal => Esquema memoria virtual y algoritmo
*/
