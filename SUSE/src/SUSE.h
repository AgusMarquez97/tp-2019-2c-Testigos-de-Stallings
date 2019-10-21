/*
 * SUSE.h
 *
 *  Created on: Sep 8, 2019
 *      Author: agus
 */

#ifndef SUSE_H_
#define SUSE_H_

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

#include "biblioteca/mensajesSuse.h"

#define pathConfig "/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/SUSE/config/configuracion.txt"

char ip[46];
char puerto[10];

int metricsTimer;
int maxMultiprog;
float alphaSJF;

char ** semIds;
char ** semInit;
char ** semMax;

t_queue* colaNews; //hilos que no pudieron entrar al ready
t_dictionary *readys; //diccionario con colas ready. KEY= processId VALUE= colaReady

void liberarVariablesGlobales();
void levantarConfig();

void levantarServidorFUSE();
void rutinaServidor(int socketRespuesta);

#endif /* SUSE_H_ */
