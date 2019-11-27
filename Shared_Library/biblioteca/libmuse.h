#ifndef BIBLIOTECA_LIBMUSE_H_
#define BIBLIOTECA_LIBMUSE_H_

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>

#include "utils.h"
#include "sockets.h"
#include "mensajesMuse.h"
#include "libmuse.h"

#define gettid() syscall(SYS_gettid)

#define LONG_MAX_PUERTO 7
#define LONG_MAX_IP 16

char puerto_muse[LONG_MAX_PUERTO];
char ip_muse[LONG_MAX_IP];
int id_muse;

pthread_mutex_t mutex_malloc;


/*
 * Inicializa la biblioteca de MUSE.
 * @param id El Process (o Thread) ID para identificar el caller en MUSE.
 * @param ip IP en dot-notation de MUSE.
 * @param puerto Puerto de conexión a MUSE.
 * @return Si pasa un error, retorna -1. Si se inicializó correctamente, retorna 0.
 * @see Para obtener el id de un proceso pueden usar getpid() de la lib POSIX (unistd.h)
 * @note Debido a la naturaleza centralizada de MUSE, esta función deberá definir
 *  el ID del proceso/hilo según "IP-ID".
 */
int muse_init(int id, char* ip, int puerto); // ==> Validada OK!
/*
 * Cierra la biblioteca de MUSE.
 */
void muse_close(); // ==> Validada OK!
/*
 * Reserva una porción de memoria contígua de tamaño 'tam'.
 * @param tam La cantidad de bytes a reservar.
 * @return La dirección de la memoria reservada.
 */
uint32_t muse_alloc(uint32_t tam); // ==> Validada OK!
/*
 * Libera una porción de memoria reservada.
 * @param dir La dirección de la memoria a reservar.
 */
void muse_free(uint32_t dir); // ==> Validada OK!
/*
 * Copia una cantidad 'n' de bytes desde una posición de memoria de MUSE a una 'dst' local.
 * @param dst Posición de memoria local con tamaño suficiente para almacenar 'n' bytes.
 * @param src Posición de memoria de MUSE de donde leer los 'n' bytes.
 * @param n Cantidad de bytes a copiar.
 * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
 */
int muse_get(void* dst, uint32_t src, size_t n); // ==> Validada OK! Recordar que la memoria local donde almaceno debe tener el malloc
/*
 * Copia una cantidad 'n' de bytes desde una posición de memoria local a una 'dst' en MUSE.
 * @param dst Posición de memoria de MUSE con tamaño suficiente para almacenar 'n' bytes.
 * @param src Posición de memoria local de donde leer los 'n' bytes.
 * @param n Cantidad de bytes a copiar.
 * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
 */
int muse_cpy(uint32_t dst, void* src, int n); // ==> Validada OK!
/*
 * Devuelve un puntero a una posición mappeada de páginas por una cantidad 'length' de bytes el archivo del 'path' dado.
 * @param path Path a un archivo en el FileSystem de MUSE a mappear.
 * @param length Cantidad de bytes de memoria a usar para mappear el archivo.
 * @param flags
 *          MAP_PRIVATE     Solo un proceso/hilo puede mappear el archivo.
 *          MAP_SHARED      El segmento asociado al archivo es compartido.
 * @return Retorna la posición de memoria de MUSE mappeada.
 * @note: Si 'length' sobrepasa el tamaño del archivo, toda extensión deberá estar llena de "\0".
 * @note: muse_free no libera la memoria mappeada. @see muse_unmap
 */
uint32_t muse_map(char *path, size_t length, int flags);
/*
 * Descarga una cantidad 'len' de bytes y lo escribe en el archivo en el FileSystem.
 * @param addr Dirección a memoria mappeada.
 * @param len Cantidad de bytes a escribir.
 * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
 * @note Si 'len' es menor que el tamaño de la página en la que se encuentre, se deberá escribir la página completa.
 */
int muse_sync(uint32_t addr, size_t len);
/*
 * Borra el mappeo a un archivo hecho por muse_map.
 * @param dir Dirección a memoria mappeada.
 * @param
 * @note Esto implicará que todas las futuras utilizaciones de direcciones basadas en 'dir' serán accesos inválidos.
 * @note Solo se deberá cerrar el archivo mappeado una vez que todos los hilos hayan liberado la misma cantidad de muse_unmap que muse_map.
 * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
 */
int muse_unmap(uint32_t dir);

#endif /* BIBLIOTECA_LIBMUSE_H_ */
