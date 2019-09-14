/*
 ============================================================================
 Name        : FUSE.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "FUSE.h"

void levantarServidorFUSE()
{
	int socketRespuesta;
	pthread_t hiloAtendedor = 0;

	int socketServidor = levantarServidor(ip,puerto);

	while(1)
	{
		if((socketRespuesta = (intptr_t) aceptarConexion(socketServidor)) != -1)
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

void levantarConfig()
{
	t_config * unConfig = retornarConfig(pathConfig);

	strcpy(ip,config_get_string_value(unConfig,"IP"));
	strcpy(puerto,config_get_string_value(unConfig,"LISTEN_PORT"));

	config_destroy(unConfig);

	loggearInfoServidor(ip,puerto);
}

int main(void) {
	remove("Linuse.log");
	iniciarLog("FUSE");
	loggearInfo("Se inicia el proceso FUSE...");
	levantarConfig();
	//levantarServidorFUSE();
	liberarVariablesGlobales();
	return EXIT_SUCCESS;
}

void liberarVariablesGlobales()
{
	destruirLog();
}
