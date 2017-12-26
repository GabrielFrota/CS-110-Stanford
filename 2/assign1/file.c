#include <stdio.h>
#include <assert.h>

#include "file.h"
#include "inode.h"
#include "diskimg.h"

// remove the placeholder implementation and replace with your own
int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf)
{
	struct inode ind ;

	inode_iget(fs, inumber, &ind) ;

	int indexSetorDisco = inode_indexlookup(fs, &ind, blockNum) ;

	diskimg_readsector(fs->dfd, indexSetorDisco, buf) ;

	int tamanhoArquivoBytes = inode_getsize(&ind) ;

	//calcular quantidade de bytes válidos no setor lido

	if (tamanhoArquivoBytes % DISKIMG_SECTOR_SIZE != 0)
	{
		if (tamanhoArquivoBytes / DISKIMG_SECTOR_SIZE == blockNum)
		{
			return tamanhoArquivoBytes % DISKIMG_SECTOR_SIZE ;
		}
		else
		{
			return DISKIMG_SECTOR_SIZE ;
		}
	}
	else
	{
		return DISKIMG_SECTOR_SIZE ;
	}
}
