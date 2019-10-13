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



//Falta como recibir los sockets
//>RECIBIR()-
//conectarse con sus clientes y atender a uno
//t_mensajeSuse* mensajeRecibido= recibirOperacion(socketEmisor);
//int32_t resultado=ejecutarMensajeSocket(mensajeRecibido);
//procesarResultadoSocket(mensajeRecibido->Operacion,resultado);
//enviar a hilolay los resultados obtenidos



//---------------------------------------------------< Planificacion >----------------------------------------------------------------|

/*Administra las tareas que tiene que hacer SUSE en base a la operacion que le llego*/
int32_t ejecutarMensajeSocket (t_mensajeSuse mensajeRecibido){

	int32_t operacion= mensajeRecibido->tipoOperacion;

switch(operacion) {
		case HANDSHAKE:
			//?
			break;
		case CREATE:
			return suse_create_servidor(mensajeRecibido);
			break;
		case NEXT:
			return suse_schedule_next_servidor(mensajeRecibido->idProceso);
			break;
		case JOIN:
			return suse_join_servidor(mensajeRecibido->idHilo);
			break;
		case RETURN:
			return suse_return_servidor(mensajeRecibido->idProceso,mensajeRecibido->idHilo);
			break;
		default:
			return -1;
	}

}

//---------------------------------------------------< PLP >----------------------------------------------------------------|

/*Crea un molde de hilo y lo agrega a la cola de news (unica para todos los procesos) y al finalizar intenta mandarlo a la de readys*/
int32_t suse_create_server(t_mensajeSuse mensaje){

	t_hiloPlanificado hiloEntrante;//creo un modelo que me servira para planificar (elemento que contendran las colas de suse)
	hiloEntrante->idProceso= mensaje->idProceso;
	hiloEntrante->idHilo= mensaje->idHilo;
	hiloEntrante->rafaga= mensaje->rafaga;

	queue_push(colaNews, hiloEntrante);
	return planificar_largoPlazo(); //revisar si esta bien metido aca

}



/*Transiciona los hilos de new a Ready*/
int32_t planificar_largoPlazo(){

	int gradoMulti= config_get_int_value(pathConfig, "MAX_MULTIPROG");

if(!queue_is_empty(colaNews)){

	t_hiloPlanificado hiloEnNews= queue_pop(colaNews); //tomo un hilo en estado new

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


	/*aca cuando ya tiene todo no habria que intentar ver si puedo meter algun new? Porque el gradoMulti bajó en la cola de este
	*proceso. O eso solo lo hace suse_create una vez lo pushea a news?.
	 Algo asi:
	 revisar_newsEsperando();
	*/

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

	t_hiloPlanificado hiloBuscado=-1;
	t_queue* colaReady= dictionary_get(readys, idProceso); //busca en el dic. la colaReady del proceso solicitante

	//en proceso

	return hiloBuscado;
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

			if((hiloAtendedor = makeDetachableThread(rutinaServidor,(void*)(intptr_t)socketRespuesta)) != 0)
			{
			}
			else
			{
				loggearError("Error al crear un nuevo hilo");
			}
		}
	}

}

void rutinaServidor(int socketRespuesta)
{
	loggearInfo(":)");
	sleep(5);
	/*
	 * Aca viene la magia...
	 * enviar y recibir mierda
	 */
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

int main(void) {
	remove("Linuse.log");
	iniciarLog("SUSE");
	loggearInfo("Se inicia el proceso SUSE...");
	levantarConfig();
	//levantarServidorSUSE();
	printf("jeje");
	liberarVariablesGlobales();
	return EXIT_SUCCESS;
}
