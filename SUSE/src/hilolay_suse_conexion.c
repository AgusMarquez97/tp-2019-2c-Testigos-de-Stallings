
#include <hilolay/alumnos.h> //wtf? Ver Issue en sisoputnfrba sobre esto
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <biblioteca/mensajesSuse.h>

/* Lib implementation: It'll only schedule the last thread that was created */
int max_tid = 0;

//--------Variables---------//
#define gettid() syscall(SYS_gettid)
#define LONG_MAX_PUERTO 7
#define LONG_MAX_IP 16

char puerto_suse[LONG_MAX_PUERTO];
char ip_suse[LONG_MAX_IP];
int id_suse;
//-----------------------//

/*suse_init (provisional):
 * 1) suse_init no esta declarado (dijeron los ayudantes que se les paso) asi que suponemos que es parecido a muse_init por ahora
 * 2)La parte de arriba de los define y las variables tendria que ir en un .h como libmuse
 */
int suse_init(int id, char* ip, int puerto) {

		iniciarLog("Linuse");
		loggearInfo("Iniciando la Biblioteca libsuse...");

		id_suse = id;
		strcpy(ip_suse, ip);
		sprintf(puerto_suse, "%d", puerto);

		int respuesta = 0;
		int socketCliente = levantarCliente(ip_suse, puerto_suse);

		if(socketCliente != -1) {
			enviarHandshake(socketCliente, id_suse);
			recibirInt(socketCliente, &respuesta);
		}
		close(socketCliente);

		if(respuesta != 1) {
			loggearError("No se ha podido iniciar la Biblioteca libsuse");
			return -1;
		}

		loggearInfo("Biblioteca libsuse iniciada con éxito");
		return 0;

	}



int suse_create(int tid){
	if (tid > max_tid) max_tid = tid;

	loggearInfo("Creando hilo...");

		uint32_t estado = -1;
		int socketCliente = levantarCliente(ip_suse, puerto_suse);

		if(socketCliente != -1) {
			enviarCreate(socketCliente, id_suse, tid);
			recibirInt(socketCliente, &estado);
		}
		close(socketCliente);

		if(estado == -1) {
			loggearError("Error: No se ha podido reservar crear el hilo");
			return 0;
		}

		loggearInfo("Hilo creado exitosamente");
		return estado;

}

int suse_schedule_next(void){


	loggearInfo("Solicitando proximo hilo...");

		uint32_t proxTid = -1;
		int socketCliente = levantarCliente(ip_suse, puerto_suse);

		if(socketCliente != -1) {
			enviarNext(socketCliente, id_suse);
			recibirInt(socketCliente, &proxTid);
		}
		close(socketCliente);

		if(proxTid == -1) {
			loggearError("Error: No se ha podido obtener el siguiente hilo");
			return 0;
		}

		loggearInfo("Proximo hilo a ejecutar recibido exitosamente!");
		return proxTid;

	/*int next = max_tid;
	printf("Scheduling next item %i...\n", next);
	return next;*/
}

/*NO es para esta entrega*/
int suse_join(int tid){
	// Not supported
	return 0;
}

int suse_close(int tid){

	printf("Closed thread %i\n", tid);
	max_tid--;
	return 0;
}

static struct hilolay_operations hiloops = {
		.suse_create = &suse_create,
		.suse_schedule_next = &suse_schedule_next,
		.suse_join = &suse_join,
		.suse_close = &suse_close
};

void hilolay_init(void){
	//init_internal(&hiloops);
}
