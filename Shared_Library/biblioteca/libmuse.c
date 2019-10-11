#include "libmuse.h"

int muse_init(int id, char* ip, int puerto) {

	iniciarLog("Linuse");
	loggearInfo("Iniciando la Biblioteca MUSE...");

	id_muse = id;
	strcpy(ip_muse, ip);
	sprintf(puerto_muse, "%d", puerto);

	int respuesta = 0;
	int socketCliente = levantarCliente(ip_muse, puerto_muse);

	if(socketCliente != -1) {
		enviarHandshake(socketCliente, id_muse);
		recibirInt(socketCliente, &respuesta);
	}
	close(socketCliente);

	if(respuesta != 1) {
		loggearError("No se ha podido iniciar la Biblioteca MUSE");
		return -1;
	}

	loggearInfo("Biblioteca MUSE iniciada con éxito");
	return 0;

}

void muse_close() {

	loggearInfo("Cerrando la Biblioteca MUSE...");

	int respuesta = 0;
	int socketCliente = levantarCliente(ip_muse, puerto_muse);

	if(socketCliente != -1) {
		enviarHandshake(socketCliente, id_muse);
		recibirInt(socketCliente, &respuesta);
	}
	close(socketCliente);

	if(respuesta != 1) {
		loggearError("No se ha podido cerrar la Biblioteca MUSE");
	} else {
		loggearInfo("Biblioteca MUSE cerrada con éxito");
		destruirLog();
	}

}

uint32_t muse_alloc(uint32_t tam) {

	loggearInfo("Reservando Memoria...");

	uint32_t direccion = -1;
	int socketCliente = levantarCliente(ip_muse, puerto_muse);

	if(socketCliente != -1) {
		enviarMalloc(socketCliente, id_muse, (int32_t)tam);
		recibirUint(socketCliente, &direccion);
	}
	close(socketCliente);

	if(direccion == -1) {
		loggearError("No se ha podido reservar la memoria con éxito");
		return 0;
	}

	loggearInfo("Memoria reservado con éxito");
	return direccion;

}



/**
 * Libera una porción de memoria reservada. Loggear error en caso de fallo

void muse_free(uint32_t dir)
{
		int resultado;
		int socket_cliente = levantarCliente(ip_muse,puerto_muse);

			if(socket_cliente!=-1)
			{
				enviarFree(socket_cliente,id_muse,dir); // Envia el ID a muse
				recibirInt(socket_cliente, &resultado);
				close(socket_cliente);
			}

			if(resultado == -1)
				loggearError("Se produjo un error en el free");
			else
				loggearInfo("Memoria liberada exitosamente");
}

/**
 * Copia una cantidad `n` de bytes desde una posición de memoria de MUSE a una `dst` local.
 * Retorna 0 en caso de exitoso o -1 en caso de error => y tira la excepcion de seg fault

int muse_get(void* dst, uint32_t src, size_t n)
{
		int resultado;
		int socket_cliente = levantarCliente(ip_muse,puerto_muse);
		void * buffer;

			if(socket_cliente!=-1)
			{
				enviarGet(socket_cliente,id_muse,src,(int32_t)n); // Envia el ID a muse
				resultado = recibirVoid(socket_cliente, &buffer);
				close(socket_cliente);
			}

			if(resultado != -1)
			{
				memcpy(dst,buffer,n);
				free(buffer);
				loggearInfo("Get procesado exitosamente");
				return 0;
			}

			loggearError("Segmentation Fault");
			return raise(SIGSEGV); // Levanta el Seg Fault

}


/**
 * Copia una cantidad `n` de bytes desde una posición de memoria local a una `dst` en MUSE.
 * @param dst Posición de memoria de MUSE con tamaño suficiente para almacenar `n` bytes.
 * @param src Posición de memoria local de donde leer los `n` bytes.
 * @param n Cantidad de bytes a copiar.
 * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.

int muse_cpy(uint32_t dst, void* src, int n)
{
		int resultado;
		int socket_cliente = levantarCliente(ip_muse,puerto_muse);

		if(socket_cliente!=-1)
		{
				enviarCpy(socket_cliente,id_muse,dst,src,(int32_t)n); // Envia el ID a muse
				recibirInt(socket_cliente, &resultado); //recibo un 0 si la operacion fue exitosa
				close(socket_cliente);

			if(resultado != -1)
				{
					loggearInfo("Cpy procesado exitosamente");
					return 0;
				}

				loggearError("Segmentation Fault");
				return raise(SIGSEGV); // Levanta el Seg Fault
		}

		return -1;


}


/**
 * Devuelve un puntero a una posición mappeada de páginas por una cantidad `length` de bytes el archivo del `path` dado.
 * @param path Path a un archivo en el FileSystem de MUSE a mappear.
 * @param length Cantidad de bytes de memoria a usar para mappear el archivo.
 * @param flags
 *          MAP_PRIVATE     Solo un proceso/hilo puede mappear el archivo.
 *          MAP_SHARED      El segmento asociado al archivo es compartido.
 * @return Retorna la posición de memoria de MUSE mappeada.
 * @note: Si `length` sobrepasa el tamaño del archivo, toda extensión deberá estar llena de "\0".
 * @note: muse_free no libera la memoria mappeada. @see muse_unmap

uint32_t muse_map(char *path, size_t length, int flags)
{
		uint32_t resultado;
		int socket_cliente = levantarCliente(ip_muse,puerto_muse);
		char * contenido = leer_archivo(path,length);
		//Pendiente de revisar si le enviamos el path con la longitud a MUSE o los bytes ya leidos en libMuse

		if(socket_cliente!=-1)
		{
			enviarMap(socket_cliente, id_muse, contenido, flags);
			recibirUint(socket_cliente, &resultado); //recibo la primer posicion de memoria mappeada del archivo
			close(socket_cliente);
		}

		if(resultado != -1)
		{
			loggearInfo("Map procesado exitosamente");
			return resultado;
		}

		loggearError("Segmentation Fault");
		return raise(SIGSEGV); // Levanta el Seg Fault
}

/**
 * Descarga una cantidad `len` de bytes y lo escribe en el archivo en el FileSystem.
 * @param addr Dirección a memoria mappeada.
 * @param len Cantidad de bytes a escribir.
 * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
 * @note Si `len` es menor que el tamaño de la página en la que se encuentre, se deberá escribir la página completa.

int muse_sync(uint32_t addr, size_t len)
{
			int32_t resultado;
			int socket_cliente = levantarCliente(ip_muse,puerto_muse);
			//Pendiente de revisar si le enviamos el path con la longitud a MUSE o los bytes ya leidos en libMuse

			if(socket_cliente!=-1)
			{
				enviarSync(socket_cliente, id_muse, addr, len);
				recibirInt(socket_cliente, &resultado); //recibo 1 en caso de exito
				close(socket_cliente);
			}

			if(resultado != -1)
			{
				loggearInfo("Sync procesado exitosamente");
				return resultado;
			}

			loggearError("Segmentation Fault");
			return raise(SIGSEGV); // Levanta el Seg Fault
}

/**
 * Borra el mappeo a un archivo hecho por muse_map.
 * @param dir Dirección a memoria mappeada.
 * @param
 * @note Esto implicará que todas las futuras utilizaciones de direcciones basadas en `dir` serán accesos inválidos.
 * @note Solo se deberá cerrar el archivo mappeado una vez que todos los hilos hayan liberado la misma cantidad de muse_unmap que muse_map.
 * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.

int muse_unmap(uint32_t dir)
{
			int32_t resultado;
			int socket_cliente = levantarCliente(ip_muse,puerto_muse);

			if(socket_cliente!=-1)
			{
				enviarUnmap(socket_cliente, id_muse,dir);
				recibirInt(socket_cliente, &resultado); //recibo 1 en caso de exito
				close(socket_cliente);
			}

			if(resultado != -1)
			{
				loggearInfo("Unmap procesado exitosamente");
				return resultado;
			}

			loggearError("Segmentation Fault");
			return raise(SIGSEGV); // Levanta el Seg Fault
}
*/
