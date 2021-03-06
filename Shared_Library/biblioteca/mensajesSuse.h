/*
 * mensajesSuse.h
 *
 *  Created on: Oct 6, 2019
 *      Author: agus
 */

#ifndef BIBLIOTECA_MENSAJESSUSE_H_
#define BIBLIOTECA_MENSAJESSUSE_H_


#include "mensajes.h"

void enviarCreate(int socketReceptor, int32_t proceso, int32_t tid);
void enviarNext(int socketReceptor, int32_t proceso);
void enviarJoin(int socketReceptor, int32_t proceso, int tid);
void enviarCloseSuse(int socketReceptor, int32_t proceso, int32_t tid);
void enviarWait (int socketReceptor, int32_t proceso, int32_t tid, char * semId);
void enviarSignal (int socketReceptor, int32_t proceso, int32_t tid, char * semId);
void enviarOperacionSuse(int socket, int32_t proceso,int32_t operacion , int32_t tid, int32_t rafaga, char * semId);
t_mensajeSuse* recibirOperacionSuse(int socketEmisor);


#endif /* BIBLIOTECA_MENSAJESSUSE_H_ */
