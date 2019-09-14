#include "sockets.h"

int levantarCliente(char* servidorIP,char* servidorPuerto)
{
		int socketCliente;

		int retorno;

		estructuraConexion* servidorObjetivo;

		socketCliente = levantarSocketGenerico(servidorIP,servidorPuerto,&servidorObjetivo);

		char * mensaje = malloc(50);
	    strcpy(mensaje,"Se creo el socket: ");
		char * aux = malloc(10);
		snprintf(aux,10,"%d",socketCliente);
		strcat(mensaje,aux);

		loggearInfo(mensaje);

		retorno = conectarConServidor(&socketCliente,servidorObjetivo);

		if(retorno == -1)
		{
			close(socketCliente);
			return retorno;
		}

		strcpy(mensaje,"El socket ");
		strcat(mensaje,aux);
		strcat(mensaje," se conecto al servidor");

		loggearInfo(mensaje);

      	free(servidorObjetivo); //Ya estoy conectado
      	free(mensaje);
      	free(aux);

        return socketCliente;

}


int levantarSocketGenerico(char* servidorIP,char* servidorPuerto,estructuraConexion** servidor)
{
	loggearInfo("Creando estructuras para la conexion...");
			instanciarConexion(servidorIP,servidorPuerto,servidor);
			//imprimirDatosServidor(*servidor);
			return crearSocket(*servidor);
}


//Instancia estructuras inicales
void instanciarConexion(char * direcIP,char * puertoDesc,estructuraConexion** estructuraFinal)
{
	estructuraConexion estructuraInicial;
	limpiarEstructuraInicial(&estructuraInicial);

      if(getaddrinfo(direcIP,puertoDesc,&estructuraInicial,estructuraFinal) != 0)
      {

          //perror("Error, no se pudieron crear las estructuras");
          loggearError("Error, no se pudieron crear las estructuras");
      }

}

void limpiarEstructuraInicial(estructuraConexion* estructuraInicial)
{

    estructuraInicial->ai_family = AF_UNSPEC;
    estructuraInicial->ai_socktype = SOCK_STREAM;
    estructuraInicial->ai_flags = 0;  //Alternativa para servidor AI_PASSIVE
    estructuraInicial->ai_protocol = 0;

}

int crearSocket(estructuraConexion* estructura)
{
		int socketResultado = socket(estructura->ai_family,estructura->ai_socktype,estructura->ai_protocol);
	    if(socketResultado == -1)
	    {
	        //perror("No se pudo asignar un socket");
	        loggearError("Error al crear el socket");
	        exit(1);
	    }
	    return socketResultado;
}


int conectarConServidor(int* socketCliente,estructuraConexion* estructuraServidor)
{
	int conexionAServidor;
	conexionAServidor = connect(*socketCliente, estructuraServidor->ai_addr, estructuraServidor->ai_addrlen);
    if(conexionAServidor==-1)
        {
            //perror("No se pudo realizar la conexion");
            loggearError("No se pudo realizar la conexion");
            //close(socketCliente);
            return -1;
        }
    return conexionAServidor;

}


//---------------------------------------------EXTRAS---------------------------------------------

void imprimirDatosServidor(estructuraConexion* estructuraObjetivo)
{
    char serverIP[NI_MAXHOST]; 		//IP Server
    char serverPuerto[NI_MAXSERV];  //Puerto Servidor
	getnameinfo(estructuraObjetivo->ai_addr, sizeof(*estructuraObjetivo->ai_addr), serverIP, sizeof(serverIP), serverPuerto, sizeof(serverPuerto), NI_NUMERICHOST|NI_NUMERICSERV);

	//printf("Servidor IP: %s\n",serverIP);
	//printf("Servidor Puerto:  %s\n",serverPuerto);

	char * ip = malloc(50);
	strcpy(ip,"IP del servidor: ");
	strcat(ip,serverIP);

	char * puerto = malloc(50);
	strcpy(puerto,"Puerto de escucha: ");
	strcat(puerto,serverPuerto);

	loggearInfo(ip);
	loggearInfo(puerto);

	free(ip);
	free(puerto);

}


void imprimirDatosCliente(estructuraConexionEntrante estructuraObjetivo,socklen_t direc_tam)
{

    char clienteIP[NI_MAXHOST]; 	//IP Cliente
    char clientePuerto[NI_MAXSERV];	//Puerto Cliente

	getnameinfo((struct sockaddr *)&estructuraObjetivo, direc_tam, clienteIP, sizeof(clienteIP), clientePuerto, sizeof(clientePuerto), NI_NUMERICHOST|NI_NUMERICSERV);

	//printf("Cliente IP = %s\n",clienteIP);
	//printf("Cliente Puerto =  %s\n",clientePuerto);

	loggearInfo("Datos del cliente: ");
	loggearInfo(clienteIP);
	loggearInfo(clientePuerto);
	/*
	char * ip = malloc(50);
	strcpy(ip,"IP del cliente objetivo: ");
	strcat(ip,clienteIP);

	char * puerto = malloc(50);
	strcpy(puerto,"Puerto de escucha: ");
	strcat(puerto,clientePuerto);

	free(ip);
	free(puerto);
	*/
}

int enviar(int socketConexion, void* datosAEnviar, int32_t tamanioAEnviar){

	int bytesTotales = 0;

	loggearInfo("Enviando...");

	while (tamanioAEnviar - bytesTotales > 0) {

		int bytesEnviados = send(socketConexion, datosAEnviar + bytesTotales, tamanioAEnviar - bytesTotales, 0);

		if(bytesEnviados < 0) {
			//salirConError("No se pudieron enviar los datos al cliente", socketConexion);
			loggearError("No se pudieron enviar los datos al cliente");
		}
		bytesTotales += bytesEnviados;
	}


	char * mensaje = malloc(50);
	strcpy(mensaje,"Se enviaron: ");
	char * aux = malloc(10);
	snprintf(aux,10,"%d",bytesTotales);
	strcat(mensaje,aux);
	strcat(mensaje," bytes");


		//Luego ver de usar itoa y  -> bytesEnviados
	loggearInfo(mensaje);

	free(mensaje);
	free(aux);

	return bytesTotales; //los mando por si alguien lo necesita

}

int recibir(int socketConexion, void* buffer,int32_t tamanioARecibir) {

	if(tamanioARecibir == 0)
		return 0;

	 char * info = malloc(300);
	 char * aux = malloc(50);

	strcpy(info,"Recibiendo info del socket ");
	snprintf(aux,10,"%d",socketConexion);
	strcat(info,aux);

	loggearInfo(info);

	int bytesTotales = recv(socketConexion, buffer, tamanioARecibir, MSG_WAITALL);

	if(bytesTotales < tamanioARecibir || bytesTotales <= 0) {
		if(bytesTotales < 0){
			strcpy(info,"No se pudieron recibir los datos del cliente ");
			snprintf(aux,10,"%d",socketConexion);
			strcat(info,aux);
			strcat(info,"\n");
			loggearError(info);
		}
		else if(bytesTotales == 0) {
			strcpy(info,"Se cerro la conexion con el cliente ");
			snprintf(aux,10,"%d",socketConexion);
			strcat(info,aux);
			strcat(info,"\n");
			loggearWarning(info);

		}else
		{
			strcpy(info,"Error con el cliente ");
			snprintf(aux,10,"%d",socketConexion);
			strcat(info,aux);
			strcat(info,", se recibieron ");
			snprintf(aux,10,"%d",bytesTotales);
			strcat(info,aux);
			strcat(info," bytes");
			strcat(info," de ");
			snprintf(aux,10,"%d",tamanioARecibir);
			strcat(info,aux);
			strcat(info,"\n");
			loggearWarning(info);
		}
		free(info);
		free(aux);
		return 0;
	}


	strcpy(info,"Se recibieron: ");
	snprintf(aux,10,"%d",bytesTotales);
	strcat(info,aux);
	strcat(info," bytes");

	loggearInfo(info);

	free(info);
	free(aux);


	return bytesTotales; //los mando por si alguien lo necesita
}

//------------------ SERVIDOR-------------

void asociarPuerto(int *socketServidor,estructuraConexion* estructuraServidor)
{
    int yes = 1;
    setsockopt(*socketServidor, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)); //Valida que se pueda usar el puerto

    if(bind(*socketServidor,estructuraServidor->ai_addr,estructuraServidor->ai_addrlen)==-1)
        {
            //loggearError("No se pudo asociar el socket al puerto");

            switch(errno)
                	{
                	case EACCES:
                		loggearError("EACCES");
                		break;
                	case EADDRINUSE:
                	    		loggearError("EADDRINUSE");
                	    		break;
                	case EBADF:
                    	    		loggearError("EBADF");
                    	    		break;
                	case EINVAL:
                    	    		loggearError("EINVAL");
                    	    		break;
                	case ENOTSOCK:
                					loggearError("ENOTSOCK");
                	        	    break;

                	case EADDRNOTAVAIL:
                	    					loggearError("EADDRNOTAVAIL");
                	    	        	    break;
                   	case EFAULT:
                    	    					loggearError("EFAULT");
                    	    	        	    break;

                   	case ELOOP:
                    	    					loggearError("ELOOP");
                    	    	        	    break;
                   	case ENAMETOOLONG:
                    	    					loggearError("ENAMETOOLONG");
                    	    	        	    break;
                   	case ENOENT:
                   	        	    					loggearError("ENOENT");
                   	        	    	        	    break;
                   	case ENOMEM:
                   	        	    					loggearError("ENOMEM");
                   	        	    	        	    break;
                   	case ENOTDIR:
                   	        	    					loggearError("ENOTDIR");
                   	        	    	        	    break;
                 	case EROFS:
                   	        	    					loggearError("EROFS");
                   	        	    	        	    break;
                 	default:
                 		loggearError("Error al asociar un puerto");

                	}
            exit(1);
        }
}

void escuchar(int * socketServidor)
{
    if(listen(*socketServidor,backlog) == -1)
    {
    	loggearError("Fallo al escuchar");
        exit(1);
    }
}


int aceptarConexion(int socketServidor)
{
		int socketCliente = -1;

        estructuraConexionEntrante conexionEntrante;
        socklen_t direc_tam = sizeof(struct sockaddr_storage);

		socketCliente=accept(socketServidor,(struct sockaddr *)&conexionEntrante,&direc_tam);

	    if(socketCliente == -1)
	    {
	    	loggearError("Fallo al aceptar conexion ");
	    	return -1;
	    }

	    loggearInfo("\n-----------------Se acepta al cliente: ------------------- \n\n");
	    imprimirDatosCliente(conexionEntrante,direc_tam);

	    return socketCliente; // Retorna el socket asociado a un cliente que quiere conectarse al servidor
}

int levantarServidor(char * servidorIP, char* servidorPuerto)
{

	  	  	int socketServidor;
			estructuraConexion* servidorObjetivo;

			socketServidor = levantarSocketGenerico(servidorIP,servidorPuerto,&servidorObjetivo);
			asociarPuerto(&socketServidor,servidorObjetivo);

	        free(servidorObjetivo);

	        //system("clear");
			escuchar(&socketServidor);

	        loggearInfo("Escuchando...\n");

	        return socketServidor;
}

void definirEsperaServidor(tiempoEspera * esperaMaxima, int segundos)
{
	esperaMaxima->tv_sec = segundos;
	esperaMaxima->tv_usec = 0;
}


//Select:
void ejecutarSelect(int maxSocket,fd_set *clientes, tiempoEspera* tiempo) //Pasar null a tiempo si no importa
{
    int retornoSelect;
        retornoSelect  = select(maxSocket+1,clientes,NULL,NULL,tiempo);

         if (retornoSelect == -1) {
        	loggearError("Error al ejecutar el select: ");
            exit(4);
        }
		 if (retornoSelect == 0) {
			loggearError("Se termino el tiempo de espera del servidor...");
            exit(4);
        }

}

//Manejo de Sets:

int estaEnSet(int socketNuevo,fd_set* setSockets)
{
    return FD_ISSET(socketNuevo,setSockets);
}
void EliminarDeSet(int socketNuevo,fd_set* setSockets)
{
    FD_CLR(socketNuevo,setSockets);
}
void agregarASet(int socketNuevo,fd_set* setSockets)
{
    FD_SET(socketNuevo,setSockets);
}
void LimpiarSet(fd_set* setSockets)
{
    FD_ZERO(setSockets);
}




void loggearNuevaConexion(int socket)
{
	  char * info = malloc(strlen("Nueva conexion asignada al socket: ") + 10 + 4);
	  char * aux = malloc(10);

	  strcpy(info,"Nueva conexion asignada al socket: ");
	  sprintf(aux,"%d",socket);
	  strcat(info,aux);
	  strcat(info,"\n");

	  loggearInfo(info);

	  free(info);
      free(aux);
}
void loggearDatosRecibidos(int socket, int datosRecibidos)
{
		  char * info = malloc(strlen("Se recibieron  bytes del socket  ") + 30 + 5);
		  char * aux = malloc(30);

		  strcpy(info,"Se recibieron  ");
		  sprintf(aux,"%d",datosRecibidos);
		  strcat(info," bytes del socket ");
		  sprintf(aux,"%d",socket);
		  strcat(info,aux);
		  strcat(info,"\n");

		  loggearInfo(info);

		  free(info);
	      free(aux);
}


void loggearInfoServidor(char * IP, char * Puerto)
{

	char * ipServidor = malloc(strlen("IP del servidor: ") + strlen(IP) + 1);
	strcpy(ipServidor,"IP del servidor: ");
	strcat(ipServidor,IP);

	char * puertoServidor = malloc(strlen("Puerto de escucha: ") + strlen(Puerto) + 1);
	strcpy(puertoServidor,"Puerto de escucha: ");
	strcat(puertoServidor,Puerto);

	loggearInfo(ipServidor);
	loggearInfo(puertoServidor);

	free(ipServidor);
	free(puertoServidor);
}
