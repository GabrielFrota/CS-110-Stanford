/*
 * PrintLock.cc
 *
 *  Created on: 7 de jun de 2017
 *      Author: gabriel
 */
#include "PrintLock.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void PrintLock::lock()
{
	pthread_mutex_lock(&mutex) ;
}

void PrintLock::unlock()
{
	pthread_mutex_unlock(&mutex) ;
}

