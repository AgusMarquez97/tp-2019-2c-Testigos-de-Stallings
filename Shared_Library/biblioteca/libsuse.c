#include "libsuse.h"

/* Lib implementation: It'll only schedule the last thread that was created */



int suse_create(int tid){
	if (tid > max_tid) max_tid = tid;

	loggearInfo("Creando hilo...");

			int32_t estado = -1;
			int socketCliente = levantarCliente(ip_suse, puerto_suse);

			if(socketCliente != -1) {
				enviarCreate(socketCliente, id_proceso, tid);
				recibirInt(socketCliente, &estado);
			}
			close(socketCliente);

			if(estado == -1) {
				loggearError("Error: No se ha podido crear el hilo");
				return 0;
			}

			loggearInfo("Hilo creado exitosamente");
			return estado;





}

int suse_schedule_next(void){
	loggearInfo("Solicitando proximo hilo...");

		int32_t proxTid;
		int socketCliente = levantarCliente(ip_suse, puerto_suse);

		if(socketCliente != -1) {
			enviarNext(socketCliente, id_proceso);
			recibirInt(socketCliente, &proxTid);
		}
		close(socketCliente);

		if(proxTid == -1) {
			loggearError("Error: No se ha podido obtener el siguiente hilo");
			return 0;
		}

		loggearInfo("Proximo hilo a ejecutar recibido exitosamente!");
		return proxTid;
}

int suse_join(int tid){

	loggearInfo("Joinenando hilo...");

				int32_t estado = -1;
				int socketCliente = levantarCliente(ip_suse, puerto_suse);

				if(socketCliente != -1) {
					enviarJoin(socketCliente, id_proceso, tid);
					recibirInt(socketCliente, &estado);
				}
				close(socketCliente);

				if(estado == -1) {
					loggearError("Error: No se ha podido realizar Join");
					return 0;
				}

				loggearInfo("Join ejecutado exitosamente");
				return estado;

}

int suse_close(int tid){

	loggearInfo("Cerrando hilo...");

				int32_t estado = -1;
				int socketCliente = levantarCliente(ip_suse, puerto_suse);

				if(socketCliente != -1) {
					enviarCloseSuse(socketCliente, id_proceso, tid);
					recibirInt(socketCliente, &estado);
				}
				close(socketCliente);

				if(estado == -1) {
					loggearError("Error: No se ha podido crear el hilo");
					return 0;
				}


	printf("Closed thread %i\n", tid);
	max_tid--;
	return 0;
}

int suse_wait(int tid, char *sem_name){
	int32_t estado = -1;
	int socketCliente = levantarCliente(ip_suse, puerto_suse);

	if(socketCliente != -1) {
		enviarWait(socketCliente, id_proceso, tid, sem_name);
		recibirInt(socketCliente, &estado);
	}
	close(socketCliente);

	if(estado == -1) {
		loggearError("Error al enviar Wait");
		return 0;
	}

	loggearInfo("Se realizo la operacion wait");
	return 0;
}

int suse_signal(int tid, char *sem_name){
	int32_t estado = -1;
		int socketCliente = levantarCliente(ip_suse, puerto_suse);

		if(socketCliente != -1) {
			enviarSignal(socketCliente, id_proceso, tid, sem_name);
			recibirInt(socketCliente, &estado);
		}
		close(socketCliente);

		if(estado == -1) {
			loggearError("Error al enviar Wait");
			return 0;
		}

		loggearInfo("Se realizo la operacion wait");
		return 0;
	return 0;
}

static struct hilolay_operations hiloops = {
		.suse_create = &suse_create,
		.suse_schedule_next = &suse_schedule_next,
		.suse_join = &suse_join,
		.suse_close = &suse_close,
		.suse_wait = &suse_wait,
		.suse_signal = &suse_signal
};
void hilolay_init(void) {


		iniciarLog("Linuse");
		loggearInfo("Iniciando la Biblioteca libsuse...");

		id_proceso = getpid();//id del proceso

		t_config * unConfig = retornarConfig(pathConfig);

		strcpy(ip_suse,config_get_string_value(unConfig,"IP"));
		strcpy(puerto_suse,config_get_string_value(unConfig,"LISTEN_PORT"));
/*
		strcpy(ip_suse, ip);
		sprintf(puerto_suse, "%d", puerto);

		int respuesta = 0;
		int socketCliente = levantarCliente(ip_suse, puerto_suse);

		if(socketCliente != -1) {
			enviarHandshakeSuse(socketCliente, id_proceso);
			recibirInt(socketCliente, &respuesta);
		}
		close(socketCliente);

		if(respuesta != 1) {
			printf("No se ha podido iniciar la Biblioteca libsuse");
			loggearError("No se ha podido iniciar la Biblioteca libsuse");
			return -1;
		}
*/
		config_destroy(unConfig);
		init_internal(&hiloops);
		loggearInfo("Biblioteca libsuse iniciada con éxito");
	}

