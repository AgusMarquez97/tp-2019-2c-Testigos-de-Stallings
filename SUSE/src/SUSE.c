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


	//IF (no hay ningun hilo en exec de ese proceso)----> ver enunciado


	t_queue* colaReady= dictionary_get(readys, idProcString); //revisar
	t_hiloPlanificado* hiloSiguiente;

	//FIFO, (Pasar después a SJF)
	hiloSiguiente= queue_pop(colaReady); //el puntero desaparece? Acordarse que son punteros no variab.
	dictionary_put(readys,idProcString,colaReady);

	printf("sacamos de ready al hilo : %d",hiloSiguiente->idHilo);

	hiloSiguiente->estadoHilo=EXEC;
	//dictionary_put(execs,idProcString,hiloSiguiente);

	return hiloSiguiente->idHilo;

}



int32_t suse_close_servidor(char *  idProcString, int tid)
{
	t_hiloPlanificado* hiloParaExec;

	hiloParaExec = dictionary_get(execs,idProcString);

	dictionary_remove(execs,idProcString);

	dictionary_put(exits,idProcString,hiloParaExec);
	hiloParaExec->estadoHilo=EXIT;

	return 0;
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
		case HANDSHAKE_SUSE:
			enviarInt(socketRespuesta, 1);
			loggearInfo("Handshake exitoso");
			break;
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
			enviarInt(socketRespuesta, 1);
			break;
		case CLOSE:
			loggearInfo("Se recibio una operacion RETURN");
			result = suse_close_servidor(idProcString,mensajeRecibido->idHilo);
			enviarInt(socketRespuesta, 1);
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
	exits = dictionary_create();
}

int main(void) {
	remove("Linuse.log");
	iniciarLog("SUSE");
	loggearInfo("Se inicia el proceso SUSE...");
	levantarConfig();
	levantarEstructuras();
	//crearHilo(planificar_largoPlazo,NULL);

	levantarServidorSUSE();

	printf("jeje");
	liberarVariablesGlobales();
	return EXIT_SUCCESS;
}
