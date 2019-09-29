/*
 ============================================================================
 Name        : Linuse.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "Linuse.h"

int main(void) {
	muse_init(getpid(),"127.0.0.1",8080);
	return EXIT_SUCCESS;
}
