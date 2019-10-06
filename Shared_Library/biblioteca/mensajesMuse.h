/*
 * mensajesMuse.h
 *
 *  Created on: Oct 6, 2019
 *      Author: agus
 */

#ifndef BIBLIOTECA_MENSAJESMUSE_H_
#define BIBLIOTECA_MENSAJESMUSE_H_

#include "mensajes.h"
/*
 * MENSAJES MUSE
 */

void enviarClose(int socketReceptor,int32_t id_proceso);
void enviarMalloc(int socketReceptor,int32_t id_proceso, int32_t tam);
void enviarFree(int socketReceptor,int32_t id_proceso, uint32_t posicion);
void enviarUnmap(int socketReceptor,int32_t id_proceso, uint32_t posicion);
void enviarGet(int socketReceptor,int32_t id_proceso, uint32_t posicionMuse, int32_t cantidadBytes);
void enviarSync(int socketReceptor,int32_t id_proceso, uint32_t posicionMuse, int32_t cantidadBytes);
void enviarCpy(int socketReceptor,int32_t id_proceso,uint32_t posicionDestino, void * origen, int32_t cantidadBytes);
void enviarMap(int socketReceptor,int32_t id_proceso, char * contenidoArchivo, int32_t flag);

void enviarHandshake(int socketReceptor,int32_t id_proceso);

mensajeMuse * recibirRequestMuse(int socketEmisor);

#endif /* BIBLIOTECA_MENSAJESMUSE_H_ */
