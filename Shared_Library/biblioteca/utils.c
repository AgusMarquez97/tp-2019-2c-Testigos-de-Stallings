#include "utils.h"

int cantidadSubStrings(char ** string)
{
	int i = 0, j= 0;
	if(string)
	{
		while(string[i]!=NULL)
		{
			if(strlen(string[i]) > 0)
				j++;
			i++;
		}
	}
	return j;
}

void liberarCadenaSplit(char ** cadena)
{
	if(cadena)
	{
		int i = 0;
		while(cadena[i] != NULL)
		{
			free(cadena[i]);
			i++;
		}
		free(cadena);
	}
}

pthread_t makeDetachableThread(void* funcion, void* param)
{
	pthread_t hiloDetacheable = crearHilo(funcion,param);
	pthread_detach(hiloDetacheable);
	return hiloDetacheable;
}

pthread_t crearHilo(void* funcion, void* param)
{

	pthread_t nuevoHilo;

	pthread_create(&nuevoHilo, NULL, funcion, param);

	return nuevoHilo;

}
