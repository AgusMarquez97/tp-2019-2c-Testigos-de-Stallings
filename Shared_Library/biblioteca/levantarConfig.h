/*
 * levantarConfig.h
 *
 *  Created on: Sep 8, 2019
 *      Author: agus
 */

#ifndef LEVANTARCONFIG_H_
#define LEVANTARCONFIG_H_



#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <commons/config.h>

/*
 * Para leer los archivos de configuracion se utilizan keys. Estas keys son palabras clave dentro del archivo de config
 * que estan seguidas de un =
 * EJ:
 * archivoConfig1
 * PUERTO=8080
 * PUERTO es una key.
 */

t_config * config;

/*
 * Para crear un nuevo archivo config
 */
void crearConfig(char* path);

/*
 * Para info sobre Keys:
 */

int existeKey(char* key);
int cantidadKeys();

/*
 * Para obtener datos del archivo config (siempre se obtiene luego del =)
 */

char * obtenerString(char* key);
int obtenerInt(char* key);
long obtenerLong(char* key);
double obtenerDouble(char* key);

/*
 * Para el array el formato a leer debe ser: [lista_valores_separados_por_coma]
 * EJ: VALORES=[1,2,3,4,5]
 * Retorna char **, donde cada char * apunta a cada elemento del array
 */

char ** obtenerArray(char* key);

/*
 * Elimina la estructura config (la variable global, el archivo no se modifica)
 */


void eliminarEstructuraConfig();

/*
 * Guarda cambios en el archivo de configuracion
 */
void guardarConfig();


/*
 * Se cambiará el valor y luego se guardará el archivo config
 */
void cambiarValorConfig(char * key, char * nuevoValor);


// Usar si puede haber problemas de sincro con un solo config global por proceso
t_config * retornarConfig(char* path);



#endif /* LEVANTARCONFIG_H_ */
