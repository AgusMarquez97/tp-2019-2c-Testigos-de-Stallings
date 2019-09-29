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

char * obtener_id(int pid, int tid)
{
	char * spid = string_itoa(pid);
	char * stid = string_itoa(tid);
	char * id = malloc(strlen(spid) + strlen(stid) + strlen("-") + 1);

	strcpy(id,spid);
	strcat(id,"-");
	strcat(id,stid);

	free(spid);
	free(stid);

	return id;
}
