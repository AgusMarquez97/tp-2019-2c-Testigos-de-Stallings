#ifndef BIBLIOTECA_LIBSUSE_H_
#define BIBLIOTECA_LIBSUSE_H_


#include <hilolay/alumnos.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "mensajesSuse.h"

/* Lib implementation: It'll only schedule the last thread that was created */
int max_tid = 0;

//-Variables (meter dentro de un .h)-//
#define gettid() syscall(SYS_gettid)
#define LONG_MAX_PUERTO 7
#define LONG_MAX_IP 16

char puerto_suse[LONG_MAX_PUERTO];
char ip_suse[LONG_MAX_IP];
int id_proceso;
//---//












#endif
