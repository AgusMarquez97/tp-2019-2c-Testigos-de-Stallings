/*
 ============================================================================
 Name        : SUSE.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "SUSE.h"



//---------------------------------------------------< PLP >----------------------------------------------------------------|

/*Crea un molde de hilo y lo agrega a la cola de news (unica para todos los procesos) y al finalizar intenta mandarlo a la de readys*/
int32_t suse_create_servidor(char* idProcString, int32_t idThread){

	t_hiloPlanificado* hiloEntrante= malloc(sizeof(t_hiloPlanificado)+1);//(elemento planificable)



	hiloEntrante->idProceso=strdup(idProcString);
	hiloEntrante->idHilo= idThread;
	hiloEntrante->estadoHilo= CREATE;
	hiloEntrante->timestampEntra= 0;
	hiloEntrante->timestampSale= 0;
	hiloEntrante->timestampCreacion=(int32_t)time(NULL);
	hiloEntrante->estimado = 0;
	hiloEntrante->tiempoEnReady= 0;
	hiloEntrante->tiempoEnExec=0;

	char* auxiliarSemBloqueante = malloc(1);
	sprintf(auxiliarSemBloqueante,"");
	hiloEntrante->semBloqueante = auxiliarSemBloqueante;

	pthread_mutex_lock(&mutexNew);
	list_add(colaNews,hiloEntrante);
	pthread_mutex_unlock(&mutexNew);
	char * msj = malloc(strlen("se crea el hilo 99999999999 del proceso 9999999999999") + 1);
	sprintf(msj,"Se creo el hilo %d del proceso %s",hiloEntrante->idHilo,hiloEntrante->idProceso);
	loggearInfo(msj);
	free(msj);
	pthread_mutex_lock(&mutexNew);
	pthread_mutex_lock(&mutexReady);
	planificar_largoPlazo();
	pthread_mutex_unlock(&mutexReady);
	pthread_mutex_unlock(&mutexNew);
	return 0; //revisar

}



/*Transiciona los hilos de new a Ready*/
void planificar_largoPlazo(){
	t_hiloPlanificado* hiloEnNews;

		if(!list_is_empty(colaNews)){

			int tam= list_size(colaNews);
			hiloEnNews=list_get(colaNews,tam-1);

				if(dictionary_has_key(readys,hiloEnNews->idProceso))
				{
					t_list* colaReady= dictionary_get(readys, hiloEnNews->idProceso);//colaReady es una lista para poder aplicar sjf
					int multiprogActual = obtenerMultiprogActual();
					if(multiprogActual < maxMultiprog)
					{
						list_remove(colaNews, tam-1);
						hiloEnNews->estadoHilo=READY;
						hiloEnNews->timestampEntraEnReady= (int32_t)time(NULL);
						list_add(colaReady, hiloEnNews);
						//dictionary_put(readys, hiloEnNews->idProceso , colaReady);
						loggearInfo("Agregamos 1 hilo de New a Ready");
					}
					else loggearWarning("grado de máxima multiprogramación alcanzado");

				}
				else{
					int multiprogActual = obtenerMultiprogActual();
					if(multiprogActual < maxMultiprog)
						{

						t_list* nuevaColaReady= list_create();
						list_remove(colaNews, tam-1);
						hiloEnNews->estadoHilo=READY;
						hiloEnNews->timestampEntraEnReady= (int32_t)time(NULL);
						list_add(nuevaColaReady, hiloEnNews);

						dictionary_put(readys, hiloEnNews->idProceso , nuevaColaReady);

						loggearInfo("Se crea cola ready para el proceso nuevo y agregamos el hilo a esta");
						}
					}
		}


}
int obtenerMultiprogActual()
	{
	int multiprogActual = 0;
	void calculoMultiprog(char * proceso, t_list* cola)
		{
		multiprogActual += list_size(cola);
		}
	;
	dictionary_iterator(readys,(void *)calculoMultiprog);

	multiprogActual += dictionary_size(execs);


	return multiprogActual;
	}



//---------------------------------------------------< PCP >----------------------------------------------------------------|

int32_t suse_schedule_next_servidor(char* idProcString){


    t_list* colaReady= dictionary_get(readys, idProcString);
    t_hiloPlanificado* hiloSiguiente;
    t_hiloPlanificado* hiloActual;//este hilo estaba ejecutando, como llega un next debe volver a ready para dejarlo ejecutar al siguiente
    hiloActual = dictionary_get(execs,idProcString);

if(!list_is_empty(colaReady)){
    //FIFO, (Pasar después a SJF)
    hiloSiguiente= removerHiloConRafagaMasCorta(colaReady);
    hiloSiguiente->estadoHilo=EXEC;
    hiloSiguiente->timestampEntra = (int32_t)time(NULL);
    hiloSiguiente->tiempoEnReady+=(int32_t)time(NULL)-hiloSiguiente->timestampEntraEnReady;


        if(hiloActual != NULL)
        {
        	hiloActual->estadoHilo = READY;
        	hiloActual->timestampSale = (int32_t)time(NULL);
        	hiloActual->estimado = hiloActual->estimado*(1-alphaSJF)+(hiloActual->timestampSale-hiloActual->timestampEntra)*alphaSJF;
        	hiloActual->timestampEntraEnReady= (int32_t)time(NULL);
        	hiloActual->tiempoEnExec+=(int32_t)time(NULL)-hiloActual->timestampEntra;
        	list_add(colaReady,hiloActual);

        }
        dictionary_put(execs,idProcString,hiloSiguiente);

    char * msj = malloc(strlen("El hilo 99999999999 del proceso 9999999999999 entro en EXEC " ) + 1);
    sprintf(msj,"El hilo %d del proceso %s entro en EXEC ",hiloSiguiente->idHilo,hiloSiguiente->idProceso);
    loggearInfo(msj);
    free(msj);


    return hiloSiguiente->idHilo;
}
if(hiloActual!=NULL)
	{

	return hiloActual->idHilo;
	}


return -1;
}

t_hiloPlanificado * removerHiloConRafagaMasCorta(t_list* colaReady)
{
	t_hiloPlanificado* hiloMasCorto = list_get(colaReady,0);
	int indicieHiloCortoActual = 0;
	int indiceHiloMasCorto = 0;
		void rafagaCorta(t_hiloPlanificado* hiloCortoActual)
		{

			if(hiloCortoActual->estimado < hiloMasCorto->estimado)
			{
				hiloMasCorto = hiloCortoActual;
				indiceHiloMasCorto=indicieHiloCortoActual;
			}
			indicieHiloCortoActual++;
		}

	list_iterate(colaReady,(void*)rafagaCorta);
	return list_remove(colaReady,indiceHiloMasCorto);
}

bool hiloFinalizo(char* idProcString, int32_t tid)
{
	bool encontroHilo(t_hiloPlanificado* unHilo)
	{
		if(strcmp(unHilo->idProceso,idProcString)==0 && unHilo->idHilo == tid)
		        return true;
		        else return false;	}

	return list_any_satisfy(exits,(void*) encontroHilo);
}

int32_t suse_join_servidor(char* idProcString, int32_t tid)
{

	if(hiloFinalizo(idProcString,tid))
	{
	return 0;
	}


	t_hiloPlanificado* hiloABloquear;

	hiloABloquear= dictionary_remove(execs, idProcString);//lo quito
	hiloABloquear->estadoHilo=BLOCK;
	hiloABloquear->hiloBloqueante=tid;//lo bloqueo
	hiloABloquear->timestampSale=(int32_t) time(NULL);
	hiloABloquear->estimado = hiloABloquear->estimado*(1-alphaSJF)+(hiloABloquear->timestampSale-hiloABloquear->timestampEntra)*alphaSJF;
	hiloABloquear->tiempoEnExec+=(int32_t)time(NULL)-hiloABloquear->timestampEntra;
	//hiloABloquear->idProceso=idProcString;//bug? sin esto guarda idprocstring "1924/002" o algo asi
	list_add(blockeds,hiloABloquear);//lo meto en blockeds



    return 0;
}



void desbloquearJoin(t_hiloPlanificado* hilo)
{
	t_hiloPlanificado* hiloBloqueadoPorJoin;

		bool existeHiloBloqueado(t_hiloPlanificado* unHilo)
			{
				if((!strcmp(unHilo->idProceso,hilo->idProceso)) && (unHilo->hiloBloqueante == hilo->idHilo)) //busca el primer hilo que esta bloqueado por el hiloBloqueante del mismo proceso
				//strcmp devuelve 0 si son iguales las cadenas, por eso el !. Si son iguales y hiloB e idHilo coinciden retorna 1...
				return 1;
				return 0;
			}



					hiloBloqueadoPorJoin= list_remove_by_condition(blockeds, (void *) existeHiloBloqueado);
					if(hiloBloqueadoPorJoin!=NULL)
					{
						pthread_mutex_lock(&mutexReady);
						hiloBloqueadoPorJoin->estadoHilo=READY;
						hiloBloqueadoPorJoin->hiloBloqueante = NULL;
						t_list* colaReadyDelProc = dictionary_get(readys,hilo->idProceso);
						list_add(colaReadyDelProc,hiloBloqueadoPorJoin);
						pthread_mutex_unlock(&mutexReady);
					}

}


int32_t suse_close_servidor(char *  idProcString, int32_t tid)
{

    t_hiloPlanificado* hiloParaExit;

    hiloParaExit = dictionary_get(execs,idProcString);

    if(hiloParaExit->idHilo==tid)//y que onda si no es el id que me pidieron?

    {
    dictionary_remove(execs,idProcString);
    hiloParaExit->timestampSale = (int32_t) time(NULL);
    hiloParaExit->estimado = hiloParaExit->estimado*(1-alphaSJF)+(hiloParaExit->timestampSale-hiloParaExit->timestampEntra)*alphaSJF;
    hiloParaExit->estadoHilo=EXIT;
    hiloParaExit->tiempoEnExec+=(int32_t)time(NULL)-hiloParaExit->timestampEntra;
    list_add(exits,hiloParaExit);

    char * msj = malloc(strlen("El hilo 99999999999 del proceso 9999999999999 finalizo") + 1);
    sprintf(msj,"El hilo %d del proceso %s finalizo",hiloParaExit->idHilo,idProcString);
    loggearInfo(msj);
    free(msj);


    desbloquearJoin(hiloParaExit); //revisa si dicho hilo estaba bloqueando a otro

    pthread_mutex_lock(&mutexNew);
    pthread_mutex_lock(&mutexReady);
    planificar_largoPlazo();
    pthread_mutex_unlock(&mutexReady);
    pthread_mutex_unlock(&mutexNew);

    return 0;


    }
    else return -1;

}


/////////////////////// WAIT Y SIGNAL ////////////////////////////////////////

int32_t suse_wait_servidor(char *idProcString,int32_t idHilo,char *semId)
{
	bool semEncontrado(t_semaforoSuse* sem)
	{
		return strcmp(sem->idSem,semId)==0; //retorna true si idSem(el id del sem en la lista) es igual a semId (al que te piden)
	}
	t_semaforoSuse* semaforo= list_find(semaforos, (void*) semEncontrado);
	if(semaforo != NULL)//si encontro el semaforo en la lista...
	{
		semaforo->valorActual--;
		if(semaforo->valorActual < 0)// si queda el valor neg el hilo se bloquea
		{
			t_hiloPlanificado* hiloABloquear;
			pthread_mutex_lock(&mutexExec);
			pthread_mutex_lock(&mutexBlocked);
			hiloABloquear= dictionary_remove(execs, idProcString);
			hiloABloquear->estadoHilo=BLOCK;
			hiloABloquear->semBloqueante = semaforo->idSem;
			list_add(blockeds,hiloABloquear);
			pthread_mutex_unlock(&mutexExec);
			pthread_mutex_unlock(&mutexBlocked);
		}


		return 0;
	}
	return -1;
}

int32_t suse_signal_servidor(char *idProcString,int32_t idHilo,char *semId)
{
	bool semEncontrado(t_semaforoSuse* sem)
		{
			return strcmp(sem->idSem, semId)==0; //retorna true si idSem(el id del sem en la lista) es igual a semId (al que te piden)
		}
		t_semaforoSuse* semaforo= list_find(semaforos, (void*) semEncontrado);
		if(semaforo != NULL && (semaforo->valorActual < semaforo->valorMax))//si encontro el semaforo en la lista...
		{
			semaforo->valorActual++;
			if(semaforo->valorActual <= 0)//si queda en positivo el semaforo no esta bloqueando ningun hilo
			{
				t_hiloPlanificado* hiloADesbloquear;

				bool hiloBloqueadoPorSem(t_hiloPlanificado* unHilo)
						{

							return strcmp(unHilo->semBloqueante,semId) == 0; //busca el primer hilo que esta bloqueado por este semaforo en particular
						}
				pthread_mutex_lock(&mutexReady);
				pthread_mutex_lock(&mutexBlocked);
				hiloADesbloquear= list_remove_by_condition(blockeds, (void *) hiloBloqueadoPorSem);
				hiloADesbloquear->estadoHilo=READY;
				hiloADesbloquear->semBloqueante = NULL;
				t_list* colaReadyDelProc = dictionary_get(readys,idProcString);
				list_add(colaReadyDelProc,hiloADesbloquear);
				pthread_mutex_unlock(&mutexBlocked);
				pthread_mutex_unlock(&mutexReady);

			}


			return 0;
		}
		return -1;
}



//----------------------------Metricas--------------------------------------------------

void escribirMetricasSemaforos(){

	int i;
	t_semaforoSuse* sem;

	for(i=0;i<list_size(semaforos);i++)
		{
		sem= list_get(semaforos, i);

		char * msj = malloc(strlen("SEMAFORO= 99999999999, VALOR= 9999999999999") + 1);
		    sprintf(msj,"SEMAFORO= %s, VALOR= %d",sem->idSem,sem->valorActual);
		    log_info(metricas,msj);
		    free(msj);
		}
}

void escribirMetricasGrado(){

		    char * msj = malloc(strlen("GRADO MULTIPROGRAMACION= 99999999999") + 1);
		    sprintf(msj,"GRADO MULTIPROGRAMACION= %d",obtenerMultiprogActual());
		    log_info(metricas,msj);
		    free(msj);
}

void metricasUnPrograma(char *  idProcString){

	bool hiloEsDePrograma(t_hiloPlanificado*  hiloNew){
			if(strcmp(hiloNew->idProceso,idProcString)==0)
				return true;
				else return false;
	    }

	int cantidadNew= list_size(list_filter(colaNews, (void*)hiloEsDePrograma));

	int cantidadReady=list_size(dictionary_get(readys,idProcString));

	int cantidadExec=((dictionary_get(execs,idProcString))!=NULL) ? 1 : 0;

	int cantidadBlock= list_size(list_filter(blockeds,(void*)hiloEsDePrograma));

	 char * msj = malloc(strlen("Proceso = 99999999999 \n Hilos en New=99999999999 \n Hilos en Ready=99999999999 \n Hilos en Exec=99999999999 \n Hilos en Blocked=99999999999\n") + 1);
			    sprintf(msj,"Proceso = %s \n Hilos en New=%d \n Hilos en Ready=%d \n Hilos en Exec=%d \n Hilos en Blocked=%d\n",idProcString,cantidadNew,cantidadReady,cantidadExec,cantidadBlock);
			    log_info(metricas,msj);
			    free(msj);
}

void escribirMetricasProgramas(){

	for(int i=0;i<list_size(procesos);i++){
		pthread_mutex_lock(&mutexNew);
		pthread_mutex_lock(&mutexReady);
		pthread_mutex_lock(&mutexExec);
		pthread_mutex_lock(&mutexBlocked);
		metricasUnPrograma(list_get(procesos,i));
		pthread_mutex_unlock(&mutexBlocked);
		pthread_mutex_unlock(&mutexExec);
		pthread_mutex_unlock(&mutexReady);
		pthread_mutex_unlock(&mutexNew);
	}
}

//---------------

int32_t tiempoEjecucionProceso(char* idProcString){

	int32_t tiempoEjecucionDelProceso=0;

	bool hiloEsDePrograma(t_hiloPlanificado*  hiloNew){
		return strcmp(hiloNew->idProceso,idProcString)==0;
	}

	void calcularEjecucion(t_hiloPlanificado* hilo){
		tiempoEjecucionDelProceso+=(int32_t)time(NULL)-hilo->timestampCreacion;
	}

	list_iterate(list_filter(colaNews, (void*)hiloEsDePrograma),calcularEjecucion);


	list_iterate((dictionary_get(readys,idProcString)),calcularEjecucion);

	t_hiloPlanificado* hiloEjecutando= dictionary_get(execs,idProcString);
	if(hiloEjecutando!=NULL)
		tiempoEjecucionDelProceso+=(int32_t)time(NULL)-hiloEjecutando->timestampCreacion;

	list_iterate(list_filter(blockeds,(void*)hiloEsDePrograma),calcularEjecucion);

	//return tiempoEjecucionDelProceso;
	return tiempoEjecucionDelProceso+1;
}


void metricasUnHilo(t_hiloPlanificado* hilo){

	int32_t tiempoEjecucion= (int32_t)time(NULL)-hilo->timestampCreacion;
	int32_t tiempoEspera=hilo->tiempoEnReady;
	int32_t tiempoCPU=hilo->tiempoEnExec;
	int32_t porcentajeTiempoEjecucion=tiempoEjecucion/tiempoEjecucionProceso(hilo->idProceso);
	char * msj = malloc(strlen("Proceso=99999999999 \n Hilo = 99999999999 \n Tiempo de ejecucion=99999999999 \n Tiempo de espera=99999999999 \n Tiempo de cpu=99999999999 \n Porcentaje Tiempo ejecucion=99999999999\n") + 1);
				    sprintf(msj,"Proceso = %s \n Hilo =%d \n Tiempo de ejecucion=%d \n Tiempo de espera=%d \n Tiempo de cpu=%d \n Porcentaje Tiempo ejecucion=%d\n",hilo->idProceso,hilo->idHilo,tiempoEjecucion,tiempoEspera,tiempoCPU,porcentajeTiempoEjecucion);
				    log_info(metricas,msj);
				    free(msj);

}

void escribirMetricasHilosTotales(){

	void escribirMetricasLista(t_list* unaLista){
		for(int i=0;i<list_size(unaLista);i++){
				metricasUnHilo(list_get(unaLista,i));
			}
	}
	void escribirMetricasDiccionario(char* key, t_list* value){
			escribirMetricasLista(value);
		}
	void escribirMetricasExec(char* key, t_hiloPlanificado* value){
				metricasUnHilo(value);
			}

	escribirMetricasLista(colaNews);

	escribirMetricasLista(blockeds);

	dictionary_iterator(readys,escribirMetricasDiccionario);

	dictionary_iterator(execs,escribirMetricasExec);



}


void escribirMetricasTotales()
{
	log_info(metricas,":::::::::Metricas:::::::::");
	escribirMetricasSemaforos();
	escribirMetricasGrado();
	escribirMetricasProgramas();
	pthread_mutex_lock(&mutexNew);
	pthread_mutex_lock(&mutexReady);
	pthread_mutex_lock(&mutexExec);
	pthread_mutex_lock(&mutexBlocked);
	escribirMetricasHilosTotales();
	pthread_mutex_unlock(&mutexBlocked);
	pthread_mutex_unlock(&mutexExec);
	pthread_mutex_unlock(&mutexReady);
	pthread_mutex_unlock(&mutexNew);


}



void levantarServidorSUSE()
{
	int socketRespuesta;
	pthread_t hiloAtendedor = 0;

	int socketServidor = levantarServidor(ip,puerto);

	while(1)
	{
		if((socketRespuesta = (intptr_t)aceptarConexion(socketServidor)) != -1)
		{

			loggearNuevaConexion(socketRespuesta);

			int * p_socket = malloc(sizeof(int));
			*p_socket = socketRespuesta;


			if((hiloAtendedor = makeDetachableThread(rutinaServidor,(void*)p_socket)) != 0)
			{

			}
			else
			{
				printf("Error al crear un nuevo hilo");
				loggearError("Error al crear un nuevo hilo");
			}
		}
	}

}

void rutinaServidor(int * p_socket)
{

	int32_t result;
	int socketRespuesta = *p_socket;
	free(p_socket);
	t_mensajeSuse*  mensajeRecibido = recibirOperacionSuse(socketRespuesta);
	char * idProcString = malloc(strlen("9999999999999")+1);
	sprintf(idProcString,"%d",mensajeRecibido->idProceso);



	switch(mensajeRecibido->tipoOperacion)
		{
		case CREATE:
			loggearInfo("Se recibio una operacion CREATE");


			result= suse_create_servidor(idProcString, mensajeRecibido->idHilo);
			if(mensajeRecibido->idHilo==0){ //Para las metricasXprograma, y para que que se ejecute de una el primer hilo¿qie pasaria con muchos programas?
							list_add(procesos,idProcString);
							suse_schedule_next_servidor(idProcString);
						}
			enviarInt(socketRespuesta, result);
			break;
		case NEXT:
			loggearInfo("Se recibio una operacion NEXT");
			pthread_mutex_lock(&mutexReady);
			pthread_mutex_lock(&mutexExec);
			result=suse_schedule_next_servidor(idProcString);
			pthread_mutex_unlock(&mutexExec);
			pthread_mutex_unlock(&mutexReady);

			enviarInt(socketRespuesta, result);
			break;
		case JOIN:
			loggearInfo("Se recibio una operacion JOIN");
			pthread_mutex_lock(&mutexBlocked);
			pthread_mutex_lock(&mutexExec);
			result= suse_join_servidor(idProcString, mensajeRecibido->idHilo);
			pthread_mutex_unlock(&mutexExec);
			pthread_mutex_unlock(&mutexBlocked);

			enviarInt(socketRespuesta, result);
			break;
		case CLOSE_SUSE:
			loggearInfo("Se recibio una operacion CLOSE_SUSE");
			pthread_mutex_lock(&mutexExec);
			result = suse_close_servidor(idProcString,mensajeRecibido->idHilo);
			pthread_mutex_unlock(&mutexExec);
			//escribirMetricasTotales();
			enviarInt(socketRespuesta, result);
			break;
		case WAIT:
			loggearInfo("Se recibio una operacion WAIT");
			pthread_mutex_lock(&mutexWaitSig);
			result = suse_wait_servidor(idProcString,mensajeRecibido->idHilo,mensajeRecibido->semId);
			free(mensajeRecibido->semId);
			enviarInt(socketRespuesta, result);
			pthread_mutex_unlock(&mutexWaitSig);
			break;
		case SIGNAL:
			loggearInfo("Se recibio una operacion SIGNAL");
			pthread_mutex_lock(&mutexWaitSig);
			result = suse_signal_servidor(idProcString,mensajeRecibido->idHilo,mensajeRecibido->semId);
			free(mensajeRecibido->semId);
			enviarInt(socketRespuesta, result);
			pthread_mutex_unlock(&mutexWaitSig);
			break;
		default: //incluye el handshake
			break;
		}
	free(mensajeRecibido);
	free(idProcString);


	close(socketRespuesta);


}

void liberarVariablesGlobales()
{
	destruirLog();
	liberarCadenaSplit(semIds);
	liberarCadenaSplit(semInit);
	liberarCadenaSplit(semMax);
}

void levantarConfig()
{
	t_config * unConfig = retornarConfig(pathConfig);

	strcpy(ip,config_get_string_value(unConfig,"IP"));
	strcpy(puerto,config_get_string_value(unConfig,"LISTEN_PORT"));

	metricsTimer = config_get_int_value(unConfig,"METRICS_TIMER");
	maxMultiprog = config_get_int_value(unConfig,"MAX_MULTIPROG");

	semIds = config_get_array_value(unConfig,"SEM_IDS");

	semInit = config_get_array_value(unConfig,"SEM_INIT");
	semMax = config_get_array_value(unConfig,"SEM_MAX");

	alphaSJF = config_get_int_value(unConfig,"ALPHA_SJF");

	config_destroy(unConfig);

	loggearInfoServidor(ip,puerto);
}

void levantarEstructuras()
{
	procesos = list_create();
	colaNews = list_create();
	readys = dictionary_create();
	execs = dictionary_create();
	exits = list_create();
	blockeds = list_create();
	inicializarSemaforosPthread();
	inicializarSemaforos();
}
void inicializarSemaforos(){
	int i = 0;
	semaforos = list_create();
	while(semIds[i]!= NULL)
	{
		t_semaforoSuse* sem = malloc(sizeof(t_semaforoSuse));



		sem->idSem = strdup(semIds[i]);
		sem->valorActual = atoi(semInit[i]);
		sem->valorMax = atoi(semMax[i]);

		list_add(semaforos,sem);
		i++;
	}

}
void inicializarSemaforosPthread(){

	pthread_mutex_init(&mutexReady, NULL);
	pthread_mutex_init(&mutexExec, NULL);
	pthread_mutex_init(&mutexExec, NULL);
	pthread_mutex_init(&mutexBlocked, NULL);
	pthread_mutex_init(&mutexWaitSig, NULL);

}

int main(void) {
	remove("Linuse.log");
	remove("Metricas.log");
	iniciarLog("SUSE");
	metricas= log_create("Metricas.log", "SUSE", 0, LOG_LEVEL_INFO);
	loggearInfo("Se inicia el proceso SUSE...");
	levantarConfig();
	levantarEstructuras();

	levantarServidorSUSE();
	liberarVariablesGlobales();
	return EXIT_SUCCESS;
}
