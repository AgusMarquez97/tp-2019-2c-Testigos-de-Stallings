#include "libmuse.h"

int muse_init(int id, char* ip, int puerto) {
	char aux[150];
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
		{
		sprintf(aux,"Fallo al realizar el handshake, init duplicado para el proceso %d",id_muse);
		loggearInfo(aux);
		}
		else
		{
		sprintf(aux,"Biblioteca MUSE inicializada con éxito para el proceso %d",id_muse);
		loggearInfo(aux);
		}

		close(socketCliente);
	} else {
		sprintf(aux,"No se ha podido inicializar la biblioteca MUSE para el proceso %d",id_muse);
		loggearWarning(aux);
	}
	return retorno;

}

uint32_t muse_alloc(uint32_t tam) {
	char aux[170];
	uint32_t direccionMemoria = 0;
	int socketCliente = levantarCliente(ip_muse, puerto_muse);

	if(socketCliente != -1) {
		//loggearInfo("Reservando porción de memoria...");
		pthread_mutex_lock(&mutex_malloc);
		enviarMalloc(socketCliente, id_muse, (int32_t)tam);
		pthread_mutex_unlock(&mutex_malloc);
		recibirUint(socketCliente, &direccionMemoria); //@return: error [0] / ok [dirección de memoria]
		close(socketCliente);
		if(direccionMemoria != 0) {
			sprintf(aux,"Porción de memoria %u reservada con éxito en la direccion %u para el proceso %d",tam,direccionMemoria,id_muse);
			loggearInfo(aux);
		} else {
			sprintf(aux,"No se han podido reservar %u bytes para el proceso %d",tam,id_muse);
			loggearWarning(aux);
		}
	}


	return direccionMemoria;

}

void muse_free(uint32_t dir) {

	if(dir>0)
	{
		char aux[150];
		int32_t retorno = -1;
		int socketCliente = levantarCliente(ip_muse, puerto_muse);

		if(socketCliente != -1) {
			//loggearInfo("Liberando porción de memoria...");
			enviarFree(socketCliente, id_muse, dir);
			recibirInt(socketCliente, &retorno);
			close(socketCliente);
			//Para mas adelante: Tenemos que definir un codigo de error que nos indique la razon por la que no se pudo realizar el free (y levantar raise si hace falta)
			if(retorno != 0) {
				sprintf(aux,"Porción de memoria solicitada %u liberada con éxito para el proceso %d",dir,id_muse);
				loggearInfo(aux);
			} else {
				sprintf(aux,"No se ha podido liberar la posicion de memoria %u liberada con éxito para el proceso %d",dir,id_muse);
				loggearWarning(aux);
			}
		}
	}

}

int muse_get(void* dst, uint32_t src, size_t n) {
	if(src>0 && dst!=NULL)
	{
		int32_t retorno = -1;
		char aux[150];

		int socketCliente = levantarCliente(ip_muse, puerto_muse);
		void* buffer;

		if(socketCliente != -1) {
			//loggearInfo("Recuperando información desde memoria...");
			enviarGet(socketCliente, id_muse, src, (int32_t)n);
			retorno = recibirVoid(socketCliente, &buffer); //@return: error [-1] / ok [0]
			close(socketCliente);
			if(retorno != -1) {
					memcpy(dst, buffer, n);
					free(buffer);
					sprintf(aux,"%lu bytes de la posicion %u recuperada con éxito para el proceso %d",n,src,id_muse);
					loggearInfo(aux);
				}
			else
			{
				sprintf(aux,"Direccion %u genera en segmentation fault para el proceso %d",src,id_muse);
				loggearWarning(aux);
				raise(SIGSEGV);
			}
		}

		return retorno;// enviar la señal de SegFault (?)
	}
	raise(SIGSEGV);
	return -1;
}

int muse_cpy(uint32_t dst, void* src, int n) {
	if(dst>0 && src!=NULL)
	{
	char aux[150];
	int32_t retorno = -1;
	int socketCliente = levantarCliente(ip_muse, puerto_muse);

	if(socketCliente != -1) {
		//loggearInfo("Copiando datos en Memoria...");
		enviarCpy(socketCliente, id_muse, dst, src, (int32_t)n);
		recibirInt(socketCliente, &retorno); //@return: error [-1] / ok [0]
		close(socketCliente);
			if(retorno != -1) {
					sprintf(aux,"%d bytes copiados en memoria con éxito en la posicion %u para el proceso %d",n,dst,id_muse);
					loggearInfo(aux);
				} else {
					sprintf(aux,"Direccion %u genera en segmentation fault para el proceso %d",dst,id_muse);
					loggearWarning(aux);
					raise(SIGSEGV);
				}
	}

	return retorno;
	}
	raise(SIGSEGV);
	return -1;

}

uint32_t muse_map(char* path, size_t length, int flags) {
	if(path && length>0 && (flags==MUSE_MAP_PRIVATE || flags==MUSE_MAP_SHARED))
	{

		char aux[200];
		uint32_t direccionMemoria = 0;
		int socketCliente = levantarCliente(ip_muse, puerto_muse);

		if(socketCliente != -1) {
			enviarMap(socketCliente, id_muse, length , flags, path);
			recibirUint(socketCliente, &direccionMemoria); //@return: error [0] / ok [dirección de memoria]
			close(socketCliente);
			if(direccionMemoria != 0) {
					sprintf(aux,"%lu bytes del archivo %s mappeado con exito en la direccion %u para el proceso %d",length,path,direccionMemoria,id_muse);
					loggearInfo(aux);
				} else {
					sprintf(aux,"No se ha podido mappear el archivo %s para el proceso %d",path,id_muse);
					loggearWarning(aux);
				}
		}
		return direccionMemoria;
	}
	return 0;

}

int muse_sync(uint32_t addr, size_t len) {
	if(addr>0&&len>0)
	{
		char aux[200];
		int32_t retorno = -1;
		int socketCliente = levantarCliente(ip_muse, puerto_muse);

		if(socketCliente != -1) {
			enviarSync(socketCliente, id_muse, addr, len);
			recibirInt(socketCliente, &retorno); //@return: error [-1] / ok [0]
			close(socketCliente);
			if(retorno != -1) {
					sprintf(aux,"%lu bytes de %u sincronizada con éxito para el proceso %d",len,addr,id_muse);
					loggearInfo(aux);
				} else {
					sprintf(aux,"No se ha podido sincronizar la información en la posicion %u para el proceso %d",addr,id_muse);
					loggearWarning(aux);
				}
		}

		return retorno;
	}
	return -1;

}

int muse_unmap(uint32_t dir) {
	if(dir>0)
	{
		char aux[170];
		int32_t retorno = -1;
		int socketCliente = levantarCliente(ip_muse, puerto_muse);

		if(socketCliente != -1) {
			enviarUnmap(socketCliente, id_muse, dir);
			recibirInt(socketCliente, &retorno); //@return: error [-1] / ok [0]
			close(socketCliente);
			if(retorno != -1) {
					sprintf(aux,"Información de %u desmappeada con éxito para el proceso %d",dir,id_muse);
					loggearInfo(aux);
				} else {
					sprintf(aux,"No se ha podido eliminar el archivo de mappeado en %u para el proceso %d",dir,id_muse);
					loggearWarning(aux);
				}
		}


		return retorno;
	}
	return -1;

}

void muse_close() {
	char aux[150];
	int32_t retorno = -1;
	int socketCliente = levantarCliente(ip_muse, puerto_muse);

	if(socketCliente != -1) {
		//loggearInfo("Cerrando la biblioteca MUSE...");
		enviarClose(socketCliente, id_muse);
		recibirInt(socketCliente, &retorno); //@return: error [-1] / ok [0]
		close(socketCliente);
		if(retorno != -1) {
				sprintf(aux,"Biblioteca MUSE cerrada con éxito para el proceso %d",id_muse);
				loggearInfo(aux);

			} else {
				sprintf(aux,"No se ha podido cerrar la biblioteca para el proceso %d",id_muse);
				loggearWarning(aux);
			}
		destruirLog();
	}

}
