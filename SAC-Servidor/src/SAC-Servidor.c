
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Servidor.h"

GBlock* disco;
size_t tamDisco;
GFile* tablaNodos;
int socketRespuesta;
int indiceTabla = -1;


int indiceObjeto(char *nombre)
{
	if(indiceTabla != -1)
	{
		for (int indActual = 0; indActual <= indiceTabla; indActual++)
			if (strcmp(nombre,tablaNodos[indActual].nombre) == 0)
				return indActual;
	}

	return -1;
}

void readdir(char* path)
{

	//enviarString(socketRespuesta, ".");
	//enviarString(socketRespuesta, "..");
	char* finalizado = malloc(10);
	int enviados = 0;
	int contArray=0;
	char directorio[MAX_FILENAME_LENGTH];
	char* nombreAEnviar;// = malloc(sizeof(MAX_FILENAME_LENGTH)); //valgrind
	char** pathCortado = malloc(10*sizeof(char*));

	for(int i=0;i<=9;i++)
		pathCortado[i] = malloc(sizeof(char*));

	pathCortado=string_n_split(path, 10, "/"); // separa el path en strings

	//if(pathCortado[contArray]==NULL) // si es el punto de montaje
	//{

		for (int indActual = 0; tablaNodos[indActual].estado != 0 && indActual < MAX_FILE_NUMBER; indActual++)
		{
			if(tablaNodos[indActual].padre==NULL)
			{
				nombreAEnviar = strdup(tablaNodos[indActual].nombre);
				enviarString(socketRespuesta, nombreAEnviar);
				enviados++;
				loggearInfo("se envia el nombre del directorio");

			}
		}

	//}
	/*else	//si no es el punto de montaje
	{
		do
		{
			strcpy(directorio,pathCortado[contArray]);

			contArray++;
		}
		while(pathCortado[contArray]!=NULL);

		for (int indActual = 0; indActual <= indiceTabla; indActual++)
		{
			if(tablaNodos[indActual].padre != NULL)
			{
				if(strcmp(tablaNodos[indActual].padre->nombre, directorio)==0)
				{
					enviarString(socketRespuesta, tablaNodos[indActual].nombre);
					enviados++;
				}
			}
		}
	}*/
	strcpy(finalizado,"dou");//hago esto porque sino enviarString llora
	enviarString(socketRespuesta, finalizado);//"avisa" al cliente que termino

	free(pathCortado);

}


void rutinaServidor(t_mensajeFuse* mensajeRecibido)
{


	char * info = malloc(530);

	//char * msj;
	char* path = malloc(500);
	char* contenido= malloc(256);

		switch(mensajeRecibido->tipoOperacion)
		{
			case GETATTR:

			break;

			case READDIR:
				recibirString(socketRespuesta, &path);

				strcpy(info,"Se recibió el path: ");
				strcat(info,path);
				loggearInfo(info);

				readdir(path); 	//recibe un path, lo busca en los nodos y devuelve los que cumplen
				//free(mensajeRecibido);

			break;

			case READ:

				recibirString(socketRespuesta, &path);//recibe el nombre

				strcpy(info,"Se recibió el path: ");
				strcat(info,path);
				loggearInfo(info);


				for (int indActual = 0; tablaNodos[indActual].estado != 0 && indActual < MAX_FILE_NUMBER; indActual++)
				{
					if( strcmp(tablaNodos[indActual].nombre, path) == 0 )
						strcpy(contenido,tablaNodos[indActual].contenido);
				}
				enviarString(socketRespuesta, contenido);


			break;

			case MKDIR:

			break;

			case MKNOD:

			break;

			case WRITE:

			break;

			case RENAME:

			break;

			case UNLINK:

			break;

			case RMDIR:

			break;

			default:

			break;

		}

		free(info);
		free(path);
		return;


}

void levantarServidorFUSE()
{

	pthread_t hiloAtendedor = 0;

	int socketServidor = levantarServidor(ip,puerto);

	//char* buffer = malloc(1000);
	char * info = malloc(300);
	char * aux = malloc(50);


	socketRespuesta = (intptr_t) aceptarConexion(socketServidor);

	loggearNuevaConexion(socketRespuesta);

	t_mensajeFuse* mensajeRecibido = malloc(sizeof(t_mensajeFuse) + 500);

	mensajeRecibido = recibirOperacionFuse(socketRespuesta);
	mensajeRecibido->idHilo = hiloAtendedor;

	sprintf(info,"La operacion recibida es %d", mensajeRecibido->tipoOperacion);
	loggearInfo(info);

	while(mensajeRecibido != NULL)
	{

		//if(mensajeRecibido != NULL)
		//{
			int err = pthread_create(&hiloAtendedor, NULL, (void*) rutinaServidor, (void*) mensajeRecibido);

			if(err != 0)
				loggearInfo("Error al crear un nuevo hilo");
			else
			{
				sprintf(info,"Se crea el hilo %d",mensajeRecibido->idHilo);
				loggearInfo(info);
				pthread_join(hiloAtendedor, NULL);
				hiloAtendedor++;
				mensajeRecibido->idHilo = hiloAtendedor;
			}

		//}
		mensajeRecibido = recibirOperacionFuse(socketRespuesta);

	}

	free(info);
	free(aux);
	free(mensajeRecibido);
	close(socketRespuesta);
}

void levantarConfig()
{
	t_config * unConfig = retornarConfig(pathConfig);

	strcpy(ip,config_get_string_value(unConfig,"IP"));
	strcpy(puerto,config_get_string_value(unConfig,"LISTEN_PORT"));

	config_destroy(unConfig);

	loggearInfoServidor(ip,puerto);

}

size_t tamArchivo(char* archivo)
{
	FILE* fd = fopen(archivo, "r");

	fseek(fd, 0L, SEEK_END);
	uint32_t tam = ftell(fd);

	fclose(fd);
	return tam;
}

int main( int argc, char *argv[] )
{

	/**/

	tamDisco= tamArchivo(argv[1]);

	int discoFD = open(argv[1], O_RDWR,0);

	disco = mmap(NULL, tamDisco, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, discoFD, 0);

	disco = disco + 1 + BITMAP_SIZE_IN_BLOCKS; // mueve el puntero dos posiciones hasta la tabla de nodos

	tablaNodos = (GFile*) disco;

	remove("Linuse.log");
	iniciarLog("FUSE");
	loggearInfo("Se inicia el proceso FUSE...");
	levantarConfig();
	levantarServidorFUSE();


	/**/

	disco = disco - 1 - BITMAP_SIZE_IN_BLOCKS;

	msync(disco, tamDisco, MS_SYNC);
	munmap(disco,tamDisco);
	close(discoFD);

	return EXIT_SUCCESS;

}




