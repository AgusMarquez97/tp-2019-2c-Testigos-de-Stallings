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

/*

t_queue* news; //hilos que no pudieron entrar al ready
t_dictionary *readys; //diccionario con colas ready. C/cola representa un proceso

//hilo provisional hasta la salida de hilolay
typedef struct {
			char* procesoAsociado;
			int id;
			char* context;
			int execution_time;

} hilo_test;


void suse_init(){
	//...
	news= queue_create();
	readys = dictionary_create();
}


int _suse_create(hilo_test hilo){

	//el hardcodeo del path es temporal
	int gradoMulti= config_get_int_value("/tp-2019-2c-Testigos-de-Stallings/SUSE/config/configuracion.txt", "MAX_MULTIPROG");

	//Existe ya algun hilo en alguna cola del proceso que me mando el nuevo hilo?
		if(dictionary_has_key(readys,hilo->procesoAsociado))
		{
				t_queue* colaReady= dictionary_get(readys, hilo->procesoAsociado);

	//Me permite el grado de multiprogramacion meterme a esa cola?
				if(dictionary_size(colaReady)<=gradoMulti){

				t_queue* colaReady= dictionary_get(readys, hilo->procesoAsociado);
				queue_push(colaReady, hilo);
				dictionary_put(readys, hilo->procesoAsociado , colaReady);
				}
				else{queue_push(news, hilo);}
			return 1;
		}
		else{
			t_queue* nuevaColaReady= queue_create();
			queue_push(nuevaColaReady, hilo);
			dictionary_put(readys, hilo->procesoAsociado , nuevaColaReady);
			return 0;
			}
}

hilo_test _suse_schedule_next(int idProcesoSolicitante){

	t_queue* colaReady= dictionary_get(readys, idProcesoSolicitante);

	//esto va a dejar de ser FIFO en el futuro para ser SJF
	hilo_test proximoAEjecutar= queue_pop(colaReady);
	dictionary_put(readys, idProcesoSolicitante ,colaReady); //volve a poner la colaReady ahora sin el proximo
	revisarNews(); //intenta agregar los news que no pudieron entrar por grado de multip

	return proximoAEjecutar;
}

//mejorar
void revisarNews(){
	if(!queue_is_empty(news)){ //si tengo news esperando
		for(int i=0;i< queue_size(news);i++){
			hilo_test newEsperando= queue_pop(news);
			_suse_create(newEsperando); //?
		}
	}
}
*/


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
	liberarVariablesGlobales();
	return EXIT_SUCCESS;
}
