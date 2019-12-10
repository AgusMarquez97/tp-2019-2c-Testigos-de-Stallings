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
#include <semaphore.h>

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
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */

#include "biblioteca/mensajesSuse.h"

#define pathConfig "/home/utnso/workspace/tp-2019-2c-Testigos-de-Stallings/SUSE/config/configuracion.txt"

char ip[46];
char puerto[10];

int metricsTimer;
int maxMultiprog;
double alphaSJF;


char ** semIds;
char ** semInit;
char ** semMax;

pthread_mutex_t mutexNew, mutexReady, mutexExec, mutexBlocked, mutexWaitSig, mutexExit, mutexProc,mutexSemHilosDisp, mutexSemaforos, mutexEstimado;
sem_t semPruebas;

t_log* metricas;

t_list* colaNews; //hilos que no pudieron entrar al ready
t_dictionary *readys; //diccionario con colas ready. KEY= processId VALUE= colaReady
t_dictionary *execs; //diccionario de exec. Key = processId Value = procesoEjecutando(1 por programa según enunciado)
t_list *exits; //exit estado comun para todos los procesos, indexamos por proceso por comodidad
t_list *blockeds; //hilos bloqueados esperando para volver al ready
t_list *semaforos; //lista de t_semaforoSuse
t_list *procesos; //todos los procesos que administra SUSE.
t_dictionary *semHilosDisp;//Diccionario que tiene un semContador por proceso, el contador nos dice cuantos hay en ready + 1 o 0 en exec, si esta en 0 el contador se bloquea y no hace el schedule next
//
int32_t suse_create_servidor(char* idProcString, int32_t idThread);


void planificar_largoPlazo();
int obtenerMultiprogActual();



int32_t suse_schedule_next_servidor(char* idProcString);
t_hiloPlanificado * removerHiloConRafagaMasCorta(t_list* colaReady);
void revisar_newsEsperando();
int32_t suse_close_servidor(char* idProcString, int32_t tid);
int32_t suse_join_servidor(char* idProcString, int32_t tid);
bool hiloFinalizo(char* idProcString,int32_t tid);
int32_t suse_wait_servidor(char* idProcString,int32_t idHilo,char *semId);
int32_t suse_signal_servidor(char* idProcString,int32_t idHilo,char *semId);

void inicializarSemaforosPthread();

//------METRICAS.....
long long timestampEnMilisegundos();
void escribirMetricasSemaforos();
void escribirMetricasGrado();
void metricasUnPrograma(char *  idProcString);
void escribirMetricasProgramas();
int32_t tiempoEjecucionProceso(char* idProcString);
void metricasUnHilo(t_hiloPlanificado* hilo);
void escribirMetricasHilosTotales();
void escribirMetricasTotales();



//
void liberarVariablesGlobales();
void levantarEstructuras();
void levantarConfig();
void inicializarSemaforos();

void levantarServidorFUSE();
void rutinaServidor(int * socketRespuesta);

#endif /* SUSE_H_ */
