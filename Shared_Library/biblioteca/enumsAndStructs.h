/*
 * enumsAndStructs.h
 *
 *  Created on: Sep 8, 2019
 *      Author: agus
 */

#ifndef ENUMSANDSTRUCTS_H_
#define ENUMSANDSTRUCTS_H_

typedef enum {

	HANDSHAKE = 1 /* Pendiente de definir que tipos de envios hay*/

} tipoEnvio;

// IMPORTANTE: DEFINIR QUE VAMOS A ENVIAR Y RECIBIR -> SE VAN A COMUNICAR ENTRE SI SUSE-MUSE-FUSE?

typedef struct {

	int32_t tipoEnvio;
	/*
	 * atributos a definir
	 */
}mensaje;


#endif /* ENUMSANDSTRUCTS_H_ */
