/*
 * exit-utils.h
 *
 *  Created on: 5 de mar de 2017
 *      Author: gabriel
 *
 *
 *  Esse header se encontra nos computadores de stanford, e aparece em varios exemplos da matéria.
 *  Não o encontrei disponivel na internet, portanto implementei as funções pelo que pude deduzir delas. Me parece
 *  que está certo.
 */

#ifndef EXIT_UTILS_H_
#define EXIT_UTILS_H_

#include <stdio.h>
#include <stdlib.h>

int exitIf( int condicional, int codigoErro, FILE* streamMsgErro, char *stringErro, ... ) ;

int exitUnless(int condicional, int codigoErro, FILE* streamMsgErro, char *stringErro, ... );


#endif /* EXIT_UTILS_H_ */
