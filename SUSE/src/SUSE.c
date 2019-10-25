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
int32_t suse_create_servidor(int32_t idProc, int32_t idThread){

	t_hiloPlanificado* hiloEntrante= malloc(sizeof(hiloEntrante));//(elemento planificable)

	hiloEntrante->idProceso= idProc;
	hiloEntrante->idHilo= idThread;



	queue_push(colaNews,hiloEntrante);
	char * msj = malloc(strlen("se crea el hilo 99999999999 del proceso 9999999999999") + 1);
	sprintf(msj,"Se creo el hilo %d del proceso %d",idThread,idProc);
	loggearInfo(msj);
	free(msj);

	return 0; //revisar

}



/*Transiciona los hilos de new a Ready*/
int32_t planificar_largoPlazo(){

	int gradoMulti= config_get_int_value(pathConfig, "MAX_MULTIPROG");

if(!queue_is_empty(colaNews)){

	t_hiloPlanificado* hiloEnNews=queue_pop(colaNews); //tomo un hilo en estado new

		if(dictionary_has_key(readys,hiloEnNews->idProceso)) //si ya hay una colaReady para el proceso de ese hilo...
		{
				t_queue* colaReady= dictionary_get(readys, hiloEnNews->idProceso); //la pido

			if(queue_size(colaReady)<=gradoMulti){ //Me permite el grado de multiprogramacion meter + procesos a esa cola?

				queue_push(colaReady, hiloEnNews);
				dictionary_put(readys, hiloEnNews->idProceso , colaReady);
		}
			else{ queue_push(colaNews, hiloEnNews); } //si no me permite el gradoMulti lo regreso a news

			return 1;
		}
		else{ //en cambio, si el proceso nunca planifico ningun hilo le crea una cola...

			t_queue* nuevaColaReady= queue_create();

			queue_push(nuevaColaReady, hiloEnNews);

			dictionary_put(readys, hiloEnNews->idProceso , nuevaColaReady);
			return 0;
			}
}
return 2;

}

//---------------------------------------------------< PCP >----------------------------------------------------------------|

int32_t suse_schedule_next_servidor(int idProceso){

	t_queue* colaReady= dictionary_get(readys, idProceso); //busca en el dic. la colaReady del proceso solicitante

	//FIFO
	int proximoAEjecutar= queue_peek(colaReady);//peek solo consulta el primero en la cola. NEXT es solo una funcion de consulta


	/*aca cuando ya tiene completo no habria que intentar ver si puedo meter algun new? Porque el gradoMulti bajó en la cola de este
	*proceso. O eso solo lo hace suse_create una vez lo pushea a news?.
	 Algo asi:
	*/
	 revisar_newsEsperando();
	/*
	 *
	 */

	return proximoAEjecutar;
}

//Revisaria todos los hilos. Por ahi no es necesario pero no pierde nada intentando (por ahi cpu al pedo, pero que tanto?)
void revisar_newsEsperando(){

	for(int i=0;i<=queue_size(colaNews);i++){
		planificar_largoPlazo();
	}
}


int32_t suse_return(int idProceso, int tid){

	return 1;
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
	char  * msj;
	int result;
	int socketRespuesta = *p_socket;
	free(p_socket);
	t_mensajeSuse*  mensajeRecibido = recibirOperacionSuse(socketRespuesta);

	switch(mensajeRecibido->tipoOperacion)
		{
		case HANDSHAKE_SUSE:
			enviarInt(socketRespuesta, 1);
			loggearInfo("Handshake exitoso");
			break;
		case CREATE:
			loggearInfo("Se recibio una operacion CREATE");
			result= suse_create_servidor(mensajeRecibido->idProceso, mensajeRecibido->idHilo);
			enviarInt(socketRespuesta, result);
			break;
		case NEXT:
			loggearInfo("Se recibio una operacion NEXT");
			result=suse_schedule_next_servidor(mensajeRecibido->idProceso);
			enviarInt(socketRespuesta, result);
			break;
		case JOIN:
			loggearInfo("Se recibio una operacion JOIN");
			enviarInt(socketRespuesta, 1);
			break;
		case RETURN:
			loggearInfo("Se recibio una operacion RETURN");
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
}

int main(void) {
	remove("Linuse.log");
	iniciarLog("SUSE");
	loggearInfo("Se inicia el proceso SUSE...");
	levantarConfig();
	levantarEstructuras();
	levantarServidorSUSE();
	printf("jeje");
	liberarVariablesGlobales();
	return EXIT_SUCCESS;
}
