#ifndef _FILE_H_
#define _FILE_H_

#include "unixfilesystem.h"

/**
 * Fetches the specified file block from the specified inode.
 * Returns the number of valid bytes in the block, -1 on error.
 */
int file_getblock(struct unixfilesystem *fs, int inumber, int blockNo, void *buf); 

/*
 * Mesma intenção do de cima, mas ao invez de buscar inode no disco pelo inumber, passa o endereço de um inode em memoria
 */

int file_getblock_ind( struct unixfilesystem *fs, struct inode *ind, int blockNo, void *buf );

#endif // _FILE_H_
