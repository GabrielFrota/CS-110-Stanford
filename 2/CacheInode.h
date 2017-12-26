/*
 * cacheInode.h
 *
 *  Created on: 11 de jan de 2017
 *      Author: gabriel
 */

#ifndef CACHEINODE_H_
#define CACHEINODE_H_

#define INODES_POR_BLOCO (512/(sizeof(struct inode)))

void CacheInode_init(void *inicio, int tamanho) ;

struct inode *CacheInode_obterInode(int inumber) ;

void CacheInode_adicionarInode(int inumber, struct inode *ind) ;

#endif /* CACHEINODE_H_ */
