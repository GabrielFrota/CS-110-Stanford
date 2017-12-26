/*
 * cacheSetor.h
 *
 *  Created on: 11 de jan de 2017
 *      Author: gabriel
 */

#ifndef CACHESETOR_H_
#define CACHESETOR_H_

void CacheSetor_init(void *inicio, int tamanho) ;

void *CacheSetor_obterSetor(int numeroSetor) ;

void CacheSetor_adicionarSetor(int numeroSetor, void *setor) ;

#endif /* CACHESETOR_H_ */
