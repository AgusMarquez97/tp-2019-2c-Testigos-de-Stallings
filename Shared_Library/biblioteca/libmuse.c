#include "libmuse.h"

int muse_init(int id, char* ip, int puerto) {

	remove("Linuse.log");
	iniciarLog("Linuse");

	pthread_mutex_init(&mutex_malloc, NULL);

	id_muse = id;
	strcpy(ip_muse, ip);
	sprintf(puerto_muse, "%d", puerto);

	loggearInfo("Inicializando la biblioteca MUSE...");

	int32_t retorno = -1; // Hay que inicializarlo, si no nunca da error..
	int socketCliente = levantarCliente(ip_muse, puerto_muse);

	if(socketCliente != -1) {
		enviarHandshake(socketCliente, id_muse);
		recibirInt(socketCliente, &retorno);

		if(retorno == -1) //@return: error -1 / ok 0
		loggearInfo("Fallo al realizar el handshake, init duplicado");
		else
		loggearInfo("Biblioteca MUSE inicializada con éxito");

		close(socketCliente);
	} else {
		loggearError("No se ha podido inicializar la biblioteca MUSE");
	}
	return retorno;

}

uint32_t muse_alloc(uint32_t tam) {

	loggearInfo("Reservando porción de memoria...");

	uint32_t direccionMemoria = 0;
	int socketCliente = levantarCliente(ip_muse, puerto_muse);

	if(socketCliente != -1) {
		pthread_mutex_lock(&mutex_malloc);
		enviarMalloc(socketCliente, id_muse, (int32_t)tam);
		pthread_mutex_unlock(&mutex_malloc);
		recibirUint(socketCliente, &direccionMemoria); //@return: error [0] / ok [dirección de memoria]
		close(socketCliente);
	}

	if(direccionMemoria != 0) {
		loggearInfo("Porción de memoria reservada con éxito");
	} else {
		loggearError("No se ha podido reservar la porción de memoria solicitada");
	}
	return direccionMemoria;

}

void muse_free(uint32_t dir) {

	loggearInfo("Liberando porción de memoria...");

	int32_t retorno = -1;
	int socketCliente = levantarCliente(ip_muse, puerto_muse);

	if(socketCliente != -1) {
		enviarFree(socketCliente, id_muse, dir);
		recibirInt(socketCliente, &retorno);
		close(socketCliente);
	}

	//Para mas adelante: Tenemos que definir un codigo de error que nos indique la razon por la que no se pudo realizar el free (y levantar raise si hace falta)
	if(retorno != 0) {
		loggearInfo("Porción de memoria liberada con éxito");
	} else {
		loggearError("No se ha podido liberar la porción de memoria solicitada");
	}

}

int muse_get(void* dst, uint32_t src, size_t n) {

	loggearInfo("Recuperando información desde memoria...");

	int32_t retorno = -1;

	int socketCliente = levantarCliente(ip_muse, puerto_muse);
	void* buffer;

	if(socketCliente != -1) {
		enviarGet(socketCliente, id_muse, src, (int32_t)n);
		retorno = recibirVoid(socketCliente, &buffer); //@return: error [-1] / ok [0]
		close(socketCliente);
	}

	if(retorno != -1) {
		memcpy(dst, buffer, n);
		free(buffer);
		loggearInfo("Información recuperada desde memoria con éxito");
		return retorno;
	}
	loggearError("Segmentation Fault");
	return raise(SIGSEGV); // enviar la señal de SegFault (?)

}

int muse_cpy(uint32_t dst, void* src, int n) {

	loggearInfo("Copiando datos en Memoria...");

	int32_t retorno;
	int socketCliente = levantarCliente(ip_muse, puerto_muse);

	if(socketCliente != -1) {
		enviarCpy(socketCliente, id_muse, dst, src, (int32_t)n);
		recibirInt(socketCliente, &retorno); //@return: error [-1] / ok [0]
		close(socketCliente);
	}

	if(retorno != -1) {
		loggearInfo("Datos en memoria copiados con éxito");
	} else {
		loggearError("No se ha podido copiar la información a memoria");
		//Posible Seg Fault
	}
	return retorno;

}

uint32_t muse_map(char* path, size_t length, int flags) {

	uint32_t direccionMemoria;
	int socketCliente = levantarCliente(ip_muse, puerto_muse);
	char* contenido = leerDesde(path, length);
	// revisar si se le envía el path con la longitud a MUSE o los bytes ya leídos en libMuse

	if(socketCliente != -1) {
		enviarMap(socketCliente, id_muse, length ,contenido, flags);
		recibirUint(socketCliente, &direccionMemoria); //@return: error [0] / ok [dirección de memoria]
		close(socketCliente);
	}

	if(direccionMemoria != 0) {
		loggearInfo("Información mappeada con éxito");
	} else {
		loggearError("No se ha podido mappear la información");
	}
	return direccionMemoria;

}

int muse_sync(uint32_t addr, size_t len) {

	int32_t retorno;
	int socketCliente = levantarCliente(ip_muse, puerto_muse);

	if(socketCliente != -1) {
		enviarSync(socketCliente, id_muse, addr, len);
		recibirInt(socketCliente, &retorno); //@return: error [-1] / ok [0]
		close(socketCliente);
	}

	if(retorno != -1) {
		loggearInfo("Información sincronizada con éxito");
	} else {
		loggearError("No se ha podido sincronizar la información");
	}
	return retorno;

}

int muse_unmap(uint32_t dir) {

	int32_t retorno;
	int socketCliente = levantarCliente(ip_muse, puerto_muse);

	if(socketCliente != -1) {
		enviarUnmap(socketCliente, id_muse, dir);
		recibirInt(socketCliente, &retorno); //@return: error [-1] / ok [0]
		close(socketCliente);
	}

	if(retorno != -1) {
		loggearInfo("Archivo de mappeo eliminado con éxito");
	} else {
		loggearError("No se ha podido eliminar el archivo de mappeo");
	}
	return retorno;

}

void muse_close() {

	loggearInfo("Cerrando la biblioteca MUSE...");

	int32_t retorno = -1;
	int socketCliente = levantarCliente(ip_muse, puerto_muse);

	if(socketCliente != -1) {
		enviarClose(socketCliente, id_muse);
		recibirInt(socketCliente, &retorno); //@return: error [-1] / ok [0]
		close(socketCliente);
	}

	if(retorno != -1) {
		loggearInfo("Biblioteca MUSE cerrada con éxito");
		destruirLog();
	} else {
		loggearError("No se ha podido cerrar la biblioteca, servidor MUSE caido");
	}

}
