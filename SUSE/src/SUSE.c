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
