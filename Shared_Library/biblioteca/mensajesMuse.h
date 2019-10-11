#ifndef BIBLIOTECA_MENSAJESMUSE_H_
#define BIBLIOTECA_MENSAJESMUSE_H_

#include "mensajes.h"

void enviarHandshake(int socketReceptor, int32_t proceso);
void enviarMalloc(int socketReceptor, int32_t proceso, int32_t tamanio);
void enviarFree(int socketReceptor, int32_t proceso, uint32_t posicion);
void enviarGet(int socketReceptor, int32_t proceso, uint32_t posicionMuse, int32_t cantidadBytes);
void enviarCpy(int socketReceptor, int32_t proceso, uint32_t posDestino, void* origen, int32_t cantBytes);
void enviarMap(int socketReceptor, int32_t proceso, char* contenidoArchivo, int32_t flag);
void enviarSync(int socketReceptor, int32_t proceso, uint32_t posMuse, int32_t cantBytes);
void enviarUnmap(int socketReceptor, int32_t proceso, uint32_t posicion);
void enviarClose(int socketReceptor, int32_t proceso);
void enviarOperacion(int socket, int32_t proceso, int32_t operacion, uint32_t posicionMemoria,
		int32_t tamanio, void* origen, char* contenido, int32_t flag);
t_mensajeMuse* recibirOperacion(int socketEmisor);

#endif /* BIBLIOTECA_MENSAJESMUSE_H_ */
