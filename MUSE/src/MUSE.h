/*
 * MUSE.h
 *
 *  Created on: Sep 8, 2019
 *      Author: agus
 */

#ifndef MUSE_H_
#define MUSE_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <biblioteca/sockets.h>
#include <biblioteca/serializacion.h>
#include <biblioteca/mensajes.h>
#include <biblioteca/enumsAndStructs.h>
#include <biblioteca/logs.h>
#include <biblioteca/levantarConfig.h>
#include <biblioteca/utils.h>

#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/node.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>
#include <commons/bitarray.h>

#define pathConfig "/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/MUSE/config/configuracion.txt"

char ip[46];
char puerto[10];
int memorySize;
int pageSize;
int swapSize;

void levantarServidorMUSE();
void rutinaServidor(int * socketRespuesta);

void levantarConfig();
void liberarVariablesGlobales();

#endif /* MUSE_H_ */
