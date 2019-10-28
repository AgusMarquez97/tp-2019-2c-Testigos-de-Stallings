#ifndef BIBLIOTECA_LIBSUSE_H_
#define BIBLIOTECA_LIBSUSE_H_


#include <hilolay/alumnos.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "mensajesSuse.h"
#include "levantarConfig.h"

#define pathConfig "/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/SUSE/config/configuracion.txt"


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





int suse_create(int tid);
int suse_schedule_next(void);
int suse_join(int tid);
int suse_close(int tid);
int suse_wait(int tid, char *sem_name);
int suse_signal(int tid, char *sem_name);
void hilolay_init();










#endif
