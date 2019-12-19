#include <stdio.h>
#include <stdlib.h>
#include <hilolay/hilolay.h>
#include <unistd.h>

#define CANT_ITERACIONES 10000

void *tocar_solo()
{
	int i;

	printf("\nCAMPO: Golpeandose para entrar...\n");
	hilolay_yield();
	printf("\nCAMPO: Luchando para llegar al frente...\n");
	hilolay_yield();
	printf("\nPALCO: Estacionando el auto...\n");
	hilolay_yield();
	printf("\nPALCO: Haciendo fila para entrar...\n");
	hilolay_yield();
	FILE * fd = fopen("/home/utnso/montaje/prueba.txt","r+");
	fseek(fd,0L,SEEK_END);
	int tam = ftell(fd);
	//fseek(fd,0L,SEEK_SET);
	char * aux = strdup("Tocar solo...");
	fwrite(aux,strlen(aux),1,fd);
	fclose(fd);
	sleep(1);
	return 0;
}

void *preparar_solo()
{
	int i;

	printf("\nPALCO: Estacionando el auto...\n");
	hilolay_yield();
	printf("\nPALCO: Haciendo fila para entrar...\n");
	hilolay_yield();
	FILE * fd = fopen("/home/utnso/montaje/prueba.txt","r+");
	fseek(fd,0L,SEEK_END);
	int tam = ftell(fd);
	//fseek(fd,0L,SEEK_SET);
	char * aux = strdup("Preparando solo...");
	fwrite(aux,strlen(aux),1,fd);
	fclose(fd);
	sleep(1);
	return 0;
}

int main(void)
{
	struct hilolay_t palco;
	struct hilolay_t campo[3];

	hilolay_init();

	hilolay_create(&palco, NULL, &preparar_solo, NULL);

	hilolay_join(&palco);

	//hilolay_create(&campo[1], NULL, &tocar_solo, NULL);
	//hilolay_create(&campo[2], NULL, &tocar_solo, NULL);
	hilolay_create(&campo[0], NULL, &tocar_solo, NULL);
	hilolay_join(&campo[0]);
	hilolay_create(&campo[1], NULL, &preparar_solo, NULL);
	hilolay_join(&campo[1]);
	hilolay_create(&campo[2], NULL, &tocar_solo, NULL);
	hilolay_join(&campo[2]);




return hilolay_return(0);
}
