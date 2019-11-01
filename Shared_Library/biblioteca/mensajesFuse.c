
#include "mensajesFuse.h"

t_mensajeFuse* recibirOperacionFuse(int socketEmisor) {

	t_mensajeFuse* mensajeRecibido;
	int cantidadRecibida = 0;
	int32_t operacion;

	//cantidadRecibida = recibirInt(socketEmisor, &proceso);
	cantidadRecibida = recibirInt(socketEmisor, &operacion);

	if(cantidadRecibida != ( sizeof(int32_t) ) )
		return NULL;

	mensajeRecibido = malloc(sizeof(t_mensajeFuse));

	//mensajeRecibido->idProceso = proceso;
	mensajeRecibido->tipoOperacion = operacion;

	return mensajeRecibido;

}
