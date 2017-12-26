/*
 * printLocked.h
 *
 *  Created on: 7 de jun de 2017
 *      Author: gabriel
 */

#ifndef PRINTLOCK_H_
#define PRINTLOCK_H_

#include <pthread.h>

class PrintLock {

public:

	static void lock() ;

	static void unlock() ;

};

#endif /* PRINTLOCK_H_ */
