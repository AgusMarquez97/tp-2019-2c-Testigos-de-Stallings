/*
 ============================================================================
 Name        : MUSE.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "MUSE.h"

void levantarServidorMUSE()
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

void rutinaServidor(int * socketRespuesta)
{
	char aux[20];
	loggearInfo(":)");
	sprintf(aux,"%d",*socketRespuesta);
	loggearInfo(aux);
	sleep(15);
	loggearInfo(aux);

	/*
	 * Aca viene la magia...
	 * enviar y recibir mierda
	 */
}

void liberarVariablesGlobales()
{
	destruirLog();
}

void levantarConfig()
{
	t_config * unConfig = retornarConfig(pathConfig);

	strcpy(ip,config_get_string_value(unConfig,"IP"));
	strcpy(puerto,config_get_string_value(unConfig,"LISTEN_PORT"));
	memorySize = config_get_int_value(unConfig,"MEMORY_SIZE");
	pageSize = config_get_int_value(unConfig,"PAGE_SIZE");
	swapSize = config_get_int_value(unConfig,"SWAP_SIZE");

	config_destroy(unConfig);

	loggearInfoServidor(ip,puerto);
}


int main(void) {
	remove("Linuse.log");
	iniciarLog("MUSE");
	loggearInfo("Se inicia el proceso MUSE...");
	levantarConfig();
	//levantarServidorMUSE();
	liberarVariablesGlobales();
	return EXIT_SUCCESS;
}
