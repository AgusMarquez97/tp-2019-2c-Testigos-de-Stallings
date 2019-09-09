
#include "logs.h"

void iniciarLogConPath(char* path,char* nombre) {

	logger = log_create(path, nombre, 0, LOG_LEVEL_INFO);

}
void iniciarLog(char* nombre) {

	logger = log_create("Lissandra.log", nombre, 0, LOG_LEVEL_INFO);

}

void loggearInfo(char* mensaje) {

	log_info(logger, mensaje);

}

void loggearWarning(char* mensaje) {

	log_warning(logger, mensaje);

}

void loggearError(char* mensaje) {

	log_error(logger, mensaje);
	log_error(logger, "Error de errno: %s", strerror(errno));

}

t_log * retornarLogConPath(char* path,char* nombre)
{
	return log_create(path, nombre, 0, LOG_LEVEL_INFO);
}

void loggearInfoEnLog(t_log * unLog,char* mensaje)
{
	log_info(unLog, mensaje);
}

void loggearWarningEnLog(t_log * unLog,char* mensaje)
{
	log_warning(unLog, mensaje);
}


void loggearErrorEnLog(t_log * unLog,char* mensaje)
{
	log_error(unLog, mensaje);
	log_error(unLog, "Error de errno: %s", strerror(errno));
}
/*		DEFINIR UN WRAPPER PARA LOGGEAR COMO PRINTF(N ARGS)
 *
 *
 */





