#include "levantarConfig.h"



void crearConfig(char* path)
{
	config = config_create(path);
}

// FUNCIONES PARA LEER CONFIG:

char * obtenerString(char* key)
{
	return config_get_string_value(config,key);
}
int obtenerInt(char* key)
{
	return config_get_int_value(config,key);
}
long obtenerLong(char* key)
{
	return config_get_long_value(config,key);
}
double obtenerDouble(char* key)
{
	return config_get_double_value(config,key);
}

char ** obtenerArray(char* key)
{
	return config_get_array_value(config,key);
}



//PARA DATOS DEL ARCHIVO CONFIG
int existeKey(char* key)
{
	return config_has_property(config,key);
}

int cantidadKeys()
{
	return config_keys_amount(config);
}


// EXTRAS:

void eliminarEstructuraConfig()
{
	config_destroy(config);
}

void guardarConfig()
{
	config_save(config);
}

void cambiarValorConfig(char * key, char * nuevoValor)
{
	config_set_value(config,key,nuevoValor);
	guardarConfig();
}



t_config * retornarConfig(char* path)
{
	return config_create(path);
}

void modificarConfig(t_config * unConfig,char * key, char * valor)
{
	config_set_value(unConfig,key,valor);
}

void destruirConfig(t_config * unConfig)
{
	config_destroy(unConfig);
}


