/*
 * MuseAuxiliares.h
 *
 *  Created on: Oct 31, 2019
 *      Author: agus
 */

#ifndef MUSEAUXILIARES_H_
#define MUSEAUXILIARES_H_

#include "MUSE.h"


// Auxiliares:

bool marcoLibreMP(int nroMarco)
{
	return bitarray_test_bit(marcosMemoriaPrincipal,nroMarco)==0;
}

int obtenerCantidadMarcos(int tamanioPagina, int tamanioMemoria)
{
	float tam_nec = (float)tamanioMemoria/tamanioPagina;
	if(tam_nec == (int)tam_nec)
	return (int)tam_nec;
	return (int)round(tam_nec + 0.5);
}

/*
void liberarMarcos()
{
	for(int i = 0; i < cantidadMarcosMemoriaPrincipal;i++)
	{
		liberarMarco(i);
	}
}

void liberarMarco(int nroMarco)
{
	int offset = nroMarco*tamPagina;
	memset(tamMemoria + offset,0,tamPagina);

	pthread_mutex_lock(&mutex_marcos_libres);
	bitarray_clean_bit(marcosMemoriaPrincipal,nroMarco); // Se libera el marco para poder ser usado
	pthread_mutex_unlock(&mutex_marcos_libres);

}

bool estaLibre(t_bitarray * unBitArray,int nroMarco)
{
	return bitarray_test_bit(unBitArray,nroMarco)==0;
}

bool estaLlena(t_bitarray * unBitArray)
{
	for(int i = 0; i < cantidadMarcosMemoriaPrincipal;i++)
		{
			if(estaLibre(i))
			{
				return false;
			}
		}
	return true;
}

*/





#endif /* MUSEAUXILIARES_H_ */
