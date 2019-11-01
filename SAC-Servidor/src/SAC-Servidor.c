
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


void readdir(char* path, void *buffer)
{
	/*filler(buffer, ".");
	filler(buffer, "..");*/

	int contArray=0;
	char directorio[MAX_FILENAME_LENGTH];
	char** pathCortado = malloc(10*sizeof(char*));

	for(int i=0;i<=9;i++)
		pathCortado[i] = malloc(sizeof(char*));

	pathCortado=string_n_split(path, 10, "/"); // separa el path en strings

	if(pathCortado[contArray]==NULL) // si es el punto de montaje
	{

		for (int indActual = 0; indActual <= indiceTabla; indActual++)
			if(tablaNodos[indActual].padre==NULL)
				filler(buffer,tablaNodos[indActual].nombre);
	}
	else	//si no es el punto de montaje
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
					filler(buffer,tablaNodos[indActual].nombre);
			}
		}
	}

	free(pathCortado);

}
void levantarServidorFUSE()
{

	//pthread_t hiloAtendedor = 0;

	int socketServidor = levantarServidor(ip,puerto);

	//char* buffer = malloc(1000);
	char * info = malloc(300);
	char * aux = malloc(50);

	socketRespuesta = (intptr_t) aceptarConexion(socketServidor);

	loggearNuevaConexion(socketRespuesta);

	free(info);
	free(aux);
}

void rutinaServidor(int socketRespuesta)
{
	loggearInfo(":)");

	char * info = malloc(530);
	//char * msj;
	char** path = malloc(500);
	t_mensajeFuse* mensajeRecibido = malloc(sizeof(t_mensajeFuse) + sizeof(path));



	while(1)
	{
		mensajeRecibido = recibirOperacionFuse(socketRespuesta);

		if(mensajeRecibido != NULL)
		{
			switch(mensajeRecibido->tipoOperacion)
			{
				case GETATTR:

				break;

				case READDIR:
					recibirString(socketRespuesta, path);

					strcpy(info,"Se recibió el path: ");
					strcat(info,*path);
					loggearInfo(info);

					//readdir(path); 	//recibe un path, lo busca en los nodos y devuelve los que cumplen
					free(mensajeRecibido);
				break;

				case READ:

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
					//free(mensajeRecibido);
				break;

			}


		}
		else
		{
			close(socketRespuesta);
			free(info);
			free(path);
			return;
		}


	}

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
	rutinaServidor(socketRespuesta);

	/**/

	disco = disco - 1 - BITMAP_SIZE_IN_BLOCKS;

	msync(disco, tamDisco, MS_SYNC);
	munmap(disco,tamDisco);
	close(discoFD);

	return EXIT_SUCCESS;

}




