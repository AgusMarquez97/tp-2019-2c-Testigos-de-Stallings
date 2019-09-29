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

t_queue* newsEsperando; //hilos que no pueden entrar a ready
t_queue* colaReady; //hilos en ready



//$Descripcion: Inicializa recursos de suse
void suseInit(){
	newsEsperando= queue_create();
	colaReady= queue_create(); //conviene una cola?
}


//$Descripcion: Recibe un nuevo hilo, y evalua si encolarlo en ready o mantenerlo en espera (Planif. Largo Plazo)
void suseNew(int hiloId){
	
	///trabajando...
	if(queue_size(colaReady)<= config_get_int_value(unConfig, gradomultip)){
		queue_push(colaReady, hiloId);
	}
	else{
		queue_push(newsEsperando, hiloId);
		//notificarProceso(); (transparencia, ver enunciado)
	}

}

//$Descripcion: Calcula el SJF, saca al proceso de ready y lo pone a ejecutar (Planif. Corto Plazo)
void suseReadyAExec(){
	/*
	 * 1.Calculo sjf cual es el proximo hilo a ejecutar en ready
	 * 2.Una vez lo tengo Veo si el programa del hilo ganador no esta ejecutando otro hilo (mas informacion ver enunciado)
	 * 3.Si no lo esta, muevo el hilo de ready a exec (el programa tendra su propio exec, hilolay ejecuta los hilos, no SUSE)
	 * 4.Libero un espacio de la cola ready
	 * 5-llamo a la colaDeNewsEsperando para ver si agrego uno a ready

}

//$Descripcion: Realiza el calculo de estimacion entre los hilos que estan en ready
void suseSJF(){
	//Necesitamos conocer la estructura de los hilos que nos va a brindar hilolay
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
