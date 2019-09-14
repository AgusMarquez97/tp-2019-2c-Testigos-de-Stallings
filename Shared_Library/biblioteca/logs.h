/*
 * logs.h
 *
 *  Created on: 25 may. 2019
 *      Author: utnso
 */

#ifndef LOGS_H_
#define LOGS_H_

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <commons/log.h>

/*
 * El logger sera la variable global encargada de cargar cada log
 */
t_log * logger;

/*
 * Para el archivo log:
 * El nivel de log: LOG_LEVEL_INFO -> Con este nivel usaremos loggearInfo, loggearWarning y loggearError
 * El archivo se llamará Lissandra.log y se creara en el dir actual. Para crear log desde otro lado usar: iniciarLogConPath()
 * El parametro nombre se usara a la hora de insertar un registro EJ:
 * [INFO] 18:33:46:762 Nombre/(PID:TID): Se loggea un ejemplo
 * Por defecto imprimirá por pantalla (en el stdout) lo que se cargue al log (ver 3er parametro log_create)
 */

void iniciarLog(char* nombre);
void iniciarLogConPath(char* path,char* nombre); //Tambien imprime por pantalla

/*
 * loggearInfo(mensajeALoggear): Loggear normalmente, todo OK -> Color blanco (en consola) / negro
 * loggearWarning(mensajeALoggear): Loggear un Warning -> Color amarillo
 * loggearError(mensajeALoggear): Loggear un error -> Color rojo
*/

void loggearInfo(char* mensaje);
void loggearWarning(char* mensaje);
void loggearError(char* mensaje);
void loggearInfoConComentario(char* comentario, char* mensaje);

void destruirLog();
void destruirUnLog(t_log * unLog);

/*
 * Para gestion de logs no Globales
 */

t_log * retornarLogConPath(char* path,char* nombre); //Tambien imprime por pantalla

void loggearInfoEnLog(t_log * unLog,char* mensaje);
void loggearErrorEnLog(t_log * unLog,char* mensaje);
void loggearWarningEnLog(t_log * unLog,char* mensaje);

void loggearInfo3Mensajes(char * mensajePrincipal, char * mensajeAConcatenar, char * mensajeFinal);

void loggearInfoConcatenandoDosMensajes(char* mensaje1, char* mensaje2);


#endif /* LOGS_H_ */
