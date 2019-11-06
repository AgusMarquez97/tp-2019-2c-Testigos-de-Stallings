/*
 ============================================================================
 Name        : SUSE.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "SUSE.h"



//---------------------------------------------------< PLP >----------------------------------------------------------------|

/*Crea un molde de hilo y lo agrega a la cola de news (unica para todos los procesos) y al finalizar intenta mandarlo a la de readys*/
int32_t suse_create_servidor(char* idProcString, int32_t idThread){

	t_hiloPlanificado* hiloEntrante= malloc(sizeof(hiloEntrante));//(elemento planificable)


	hiloEntrante->idProceso=idProcString;
	hiloEntrante->idHilo= idThread;
	hiloEntrante->estadoHilo= CREATE;



	queue_push(colaNews,hiloEntrante);

	char * msj = malloc(strlen("se crea el hilo 99999999999 del proceso 9999999999999") + 1);
	sprintf(msj,"Se creo el hilo %d del proceso %s",idThread,idProcString);
	loggearInfo(msj);
	free(msj);
	planificar_largoPlazo();
	return 0; //revisar

}



/*Transiciona los hilos de new a Ready*/
void planificar_largoPlazo(){
	t_hiloPlanificado* hiloEnNews;

		if(!queue_is_empty(colaNews)){

				hiloEnNews=queue_peek(colaNews);

				if(dictionary_has_key(readys,hiloEnNews->idProceso))
				{
					t_queue* colaReady= dictionary_get(readys, hiloEnNews->idProceso);
					int multiprogActual = obtenerMultiprogActual();
					if(multiprogActual < maxMultiprog)
					{
						queue_pop(colaNews);
						hiloEnNews->estadoHilo=READY;
						queue_push(colaReady, hiloEnNews);
						//dictionary_put(readys, hiloEnNews->idProceso , colaReady);
						loggearInfo("Agregamos 1 hilo de New a Ready");
					}
					else loggearWarning("grado de máxima multiprogramación alcanzado");

				}
				else{
					int multiprogActual = obtenerMultiprogActual();
					if(multiprogActual < maxMultiprog)
						{

						t_queue* nuevaColaReady= queue_create();
						queue_pop(colaNews);
						hiloEnNews->estadoHilo=READY;
						queue_push(nuevaColaReady, hiloEnNews);

						dictionary_put(readys, hiloEnNews->idProceso , nuevaColaReady);

						loggearInfo("Se crea cola ready para el proceso nuevo y agregamos el hilo a esta");
						}
					}
		}


}
int obtenerMultiprogActual()
	{
	int multiprogActual = 0;
	void calculoMultiprog(char * proceso, t_queue* cola)
		{
		multiprogActual += queue_size(cola);
		}
	dictionary_iterator(readys,(void *)calculoMultiprog);
	multiprogActual += dictionary_size(execs);

	return multiprogActual;
	}



//---------------------------------------------------< PCP >----------------------------------------------------------------|

int32_t suse_schedule_next_servidor(char* idProcString){


    t_queue* colaReady= dictionary_get(readys, idProcString);
    t_hiloPlanificado* hiloSiguiente;
if(!queue_is_empty(colaReady)){
    //FIFO, (Pasar después a SJF)
    hiloSiguiente= queue_pop(colaReady);
    hiloSiguiente->estadoHilo=EXEC;
    dictionary_put(execs,idProcString,hiloSiguiente);

    char * msj = malloc(strlen("El hilo 99999999999 del proceso 9999999999999 entro en EXEC " ) + 1);
    sprintf(msj,"El hilo %d del proceso %s entro en EXEC ",hiloSiguiente->idHilo,idProcString);
    loggearInfo(msj);
    free(msj);


    return hiloSiguiente->idHilo;
}
return 0;

}
bool hiloFinalizo(char* idProcString, int32_t tid)
{
	bool encontroHilo(t_hiloPlanificado* unHilo)
	{
		return (unHilo->idProceso == idProcString && unHilo->idHilo == tid);
	}

	return list_any_satisfy(exits,(void*) encontroHilo);
}
int32_t suse_join_servidor(char* idProcString, int32_t tid)
{

	if(hiloFinalizo(idProcString,tid))
	{
	return 0;
	}


	t_hiloPlanificado* hiloABloquear;

	hiloABloquear= dictionary_remove(execs, idProcString);//lo quito
	hiloABloquear->estadoHilo=BLOCK;
	hiloABloquear->hiloBloqueante=tid;//lo bloqueo
	list_add(blockeds,hiloABloquear);//lo meto en blockeds



    return 0;
}



void desbloquearJoin(t_hiloPlanificado* hilo)
{
	t_hiloPlanificado* hiloBloqueadoPorJoin;

		bool existeHiloBloqueado(t_hiloPlanificado* unHilo)
			{
				return (unHilo->idProceso==hilo->idProceso) && unHilo->hiloBloqueante == hilo->idHilo; //busca el primer hilo que esta bloqueado por el hiloBloqueante del mismo proceso
			}

		if(list_any_satisfy(blockeds, (void*)existeHiloBloqueado)){ //existe algun hilo bloqueado por el que termino?

					hiloBloqueadoPorJoin= list_remove_by_condition(blockeds, (void *) existeHiloBloqueado);
					hiloBloqueadoPorJoin->estadoHilo=READY;
					hiloBloqueadoPorJoin->hiloBloqueante = NULL;
					t_queue* colaReadyDelProc = dictionary_get(readys,hilo->idProceso);
					queue_push(colaReadyDelProc,hiloBloqueadoPorJoin);
			}

}


int32_t suse_close_servidor(char *  idProcString, int32_t tid)
{

    t_hiloPlanificado* hiloParaExit;
    hiloParaExit = dictionary_get(execs,idProcString);

    if(hiloParaExit->idHilo==tid)//y que onda si no es el id que me pidieron?

    {
    dictionary_remove(execs,idProcString);
    hiloParaExit->estadoHilo=EXIT;
    list_add(exits,hiloParaExit);

    char * msj = malloc(strlen("El hilo 99999999999 del proceso 9999999999999 finalizo") + 1);
    sprintf(msj,"El hilo %d del proceso %s finalizo",hiloParaExit->idHilo,idProcString);
    loggearInfo(msj);
    free(msj);


    //desbloquearJoin(hiloParaExit); //revisa si dicho hilo estaba bloqueando a otro
    return 0;


    }
    else return -1;

}


//////// WAIT Y SIGNAL //////////

int32_t suse_wait_servidor(char *idProcString,int32_t idHilo,char *semId)
{
	bool semEncontrado(t_semaforoSuse* sem)
	{
		return sem->idSem == semId; //retorna true si idSem(el id del sem en la lista) es igual a semId (al que te piden)
	}
	t_semaforoSuse* semaforo= list_find(semaforos, (void*) semEncontrado);
	if(semaforo != NULL)//si encontro el semaforo en la lista...
	{
		semaforo->valorActual--;
		if(semaforo->valorActual < 0)// si queda el valor neg el hilo se bloquea
		{
			t_hiloPlanificado* hiloABloquear;

			hiloABloquear= dictionary_remove(execs, idProcString);
			hiloABloquear->estadoHilo=BLOCK;
			hiloABloquear->semBloqueante = semaforo;
			list_add(blockeds,hiloABloquear);
		}


		return 0;
	}
	return -1;
}

int32_t suse_signal_servidor(char *idProcString,int32_t idHilo,char *semId)
{
	bool semEncontrado(t_semaforoSuse* sem)
		{
			return sem->idSem == semId; //retorna true si idSem(el id del sem en la lista) es igual a semId (al que te piden)
		}
		t_semaforoSuse* semaforo= list_find(semaforos, (void*) semEncontrado);
		if(semaforo != NULL && (semaforo->valorActual < semaforo->valorMax))//si encontro el semaforo en la lista...
		{
			semaforo->valorActual++;
			if(semaforo->valorActual <= 0)//si queda en positivo el semaforo no esta bloqueando ningun hilo
			{
				t_hiloPlanificado* hiloADesbloquear;

				bool hiloBloqueadoPorSem(t_hiloPlanificado* unHilo)
						{
							return unHilo->semBloqueante == semId; //busca el primer hilo que esta bloqueado por este semaforo en particular
						}
				hiloADesbloquear= list_remove_by_condition(blockeds, (void *) hiloBloqueadoPorSem);
				hiloADesbloquear->estadoHilo=READY;
				hiloADesbloquear->semBloqueante = NULL;
				t_queue* colaReadyDelProc = dictionary_get(readys,idProcString);
				queue_push(colaReadyDelProc,hiloADesbloquear);
			}


			return 0;
		}
		return -1;
}

void levantarServidorSUSE()
{
	int socketRespuesta;
	pthread_t hiloAtendedor = 0;

	int socketServidor = levantarServidor(ip,puerto);

	while(1)
	{
		if((socketRespuesta = (intptr_t)aceptarConexion(socketServidor)) != -1)
		{
			loggearNuevaConexion(socketRespuesta);
			int * p_socket = malloc(sizeof(int));
			*p_socket = socketRespuesta;


			if((hiloAtendedor = makeDetachableThread(rutinaServidor,(void*)p_socket)) != 0)
			{

			}
			else
			{
				printf("Error al crear un nuevo hilo");
				loggearError("Error al crear un nuevo hilo");
			}
		}
	}

}

void rutinaServidor(int * p_socket)
{
	int32_t result;
	int socketRespuesta = *p_socket;
	free(p_socket);
	t_mensajeSuse*  mensajeRecibido = recibirOperacionSuse(socketRespuesta);
	char * idProcString = malloc(strlen("9999999999999")+1);
	sprintf(idProcString,"%d",mensajeRecibido->idProceso);


	switch(mensajeRecibido->tipoOperacion)
		{
		case CREATE:
			loggearInfo("Se recibio una operacion CREATE");
			result= suse_create_servidor(idProcString, mensajeRecibido->idHilo);
			enviarInt(socketRespuesta, result);
			break;
		case NEXT:
			loggearInfo("Se recibio una operacion NEXT");
			result=suse_schedule_next_servidor(idProcString);
			enviarInt(socketRespuesta, result);
			break;
		case JOIN:
			loggearInfo("Se recibio una operacion JOIN");
			result= suse_join_servidor(idProcString, mensajeRecibido->idHilo);
			enviarInt(socketRespuesta, result);
			break;
		case CLOSE_SUSE:
			loggearInfo("Se recibio una operacion CLOSE_SUSE");
			result = suse_close_servidor(idProcString,mensajeRecibido->idHilo);
			enviarInt(socketRespuesta, result);
			break;
		case WAIT:
			loggearInfo("Se recibio una operacion WAIT");
			result = suse_wait_servidor(idProcString,mensajeRecibido->idHilo,mensajeRecibido->semId);
			enviarInt(socketRespuesta, result);
			break;
		case SIGNAL:
			loggearInfo("Se recibio una operacion SIGNAL");
			result = suse_signal_servidor(idProcString,mensajeRecibido->idHilo,mensajeRecibido->semId);
			enviarInt(socketRespuesta, result);
			break;
		default: //incluye el handshake
			break;
		}
	free(mensajeRecibido);


	close(socketRespuesta);
}

void liberarVariablesGlobales()
{
	destruirLog();
	liberarCadenaSplit(semIds);
	liberarCadenaSplit(semInit);
	liberarCadenaSplit(semMax);
}

void levantarConfig()
{
	t_config * unConfig = retornarConfig(pathConfig);

	strcpy(ip,config_get_string_value(unConfig,"IP"));
	strcpy(puerto,config_get_string_value(unConfig,"LISTEN_PORT"));

	metricsTimer = config_get_int_value(unConfig,"METRICS_TIMER");
	maxMultiprog = config_get_int_value(unConfig,"MAX_MULTIPROG");

	semIds = config_get_array_value(unConfig,"SEM_IDS");

	semInit = config_get_array_value(unConfig,"SEM_INIT");
	semMax = config_get_array_value(unConfig,"SEM_MAX");

	alphaSJF = config_get_int_value(unConfig,"ALPHA_SJF");

	config_destroy(unConfig);

	loggearInfoServidor(ip,puerto);
}
void levantarEstructuras()
{
	colaNews = queue_create();
	readys = dictionary_create();
	execs = dictionary_create();
	exits = list_create();
	blockeds = list_create();
	inicializarSemaforos();
}
void inicializarSemaforos(){
	int i = 0;
	semaforos = list_create();
	while(semIds[i]!= NULL)
	{
		t_semaforoSuse* sem = malloc(sizeof(t_semaforoSuse));



		sem->idSem = strdup(semIds[i]);
		sem->valorActual = atoi(semInit[i]);
		sem->valorMax = atoi(semMax[i]);

		list_add(semaforos,sem);
		i++;
	}

}

int main(void) {
	remove("Linuse.log");
	iniciarLog("SUSE");
	loggearInfo("Se inicia el proceso SUSE...");
	levantarConfig();
	levantarEstructuras();

	levantarServidorSUSE();
	liberarVariablesGlobales();
	return EXIT_SUCCESS;
}
