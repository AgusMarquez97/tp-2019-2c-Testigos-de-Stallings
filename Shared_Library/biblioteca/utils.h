/*
 * utils.h
 *
 *  Created on: Sep 9, 2019
 *      Author: agus
 */

#ifndef BIBLIOTECA_UTILS_H_
#define BIBLIOTECA_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <commons/string.h>

// STRINGS
void liberarCadenaSplit(char ** cadena);
int cantidadSubStrings(char ** string);

pthread_t makeDetachableThread(void* funcion, void* param);
pthread_t crearHilo(void* funcion, void* param);

char * obtener_id(int pid, int tid);


#endif /* BIBLIOTECA_UTILS_H_ */
