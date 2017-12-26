#include <stdio.h>
#include <assert.h>

#include "inode.h"
#include "diskimg.h"

// remove the placeholder implementation and replace with your own
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp)
{
	int err ;
	inumber-- ;

	int indexSetor = inumber / INODES_POR_SETOR ;
	int indexInodeSetor = inumber % INODES_POR_SETOR ;

	struct inode inodesSetor[INODES_POR_SETOR] ;

	err = diskimg_readsector(fs->dfd, INODE_START_SECTOR + indexSetor, inodesSetor) ;

	if (err == -1)
	{
		fprintf(stderr, "inode_get(inumber=%d) retornando -1 no diskimage_readsector", inumber) ;
		return -1 ;
	}

	*inp = inodesSetor[indexInodeSetor] ;

	return 0 ;
}

// remove the placeholder implementation and replace with your own
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum)
{
	int err ;

	if ((inp->i_mode & ILARG) == 0)
	{
		//arquivo pequeno, com ponteiros diretos para blocos do arquivo
		return inp->i_addr[blockNum] ;
	}
	else
	{
		//arquivo grande, com ponteiros indiretos. 7 ponteiros de 1 nivel, 1 ponteiro de 2 niveis.

		int indexSetorPonteiros = blockNum / PONTEIROS_POR_SETOR ;
		int indexPonteiroDireto = blockNum % PONTEIROS_POR_SETOR ;

		if (indexSetorPonteiros < 7)
		{
			// pointeiro de 1 nivel

			uint16_t ponteirosDiretosArquivo[PONTEIROS_POR_SETOR] ;

			err = diskimg_readsector(fs->dfd, inp->i_addr[indexSetorPonteiros], ponteirosDiretosArquivo) ;

			if (err == -1)
			{
				fprintf(stderr, "inode_indexlookup(blockNum=%d) retornando -1 no diskimage_readsector bloco<7 \n",
						blockNum) ;
				return -1 ;
			}

			return ponteirosDiretosArquivo[indexPonteiroDireto] ;
		}
		else
		{
			// ponteiro de 2 niveis

			uint16_t bufferPonteiros[PONTEIROS_POR_SETOR] ;

			err = diskimg_readsector(fs->dfd, inp->i_addr[7], bufferPonteiros) ;

			if (err == -1)
			{
				fprintf(stderr, "inode_indexlookup(blockNum=%d) retornando -1 no diskimage_readsector ponteiro 2 niveis \n",
						blockNum) ;
				return -1 ;
			}

			indexSetorPonteiros -= 7 ; //ajusta index após descer 1 nivel

			err = diskimg_readsector(fs->dfd, bufferPonteiros[indexSetorPonteiros], bufferPonteiros) ;

			if (err == -1)
			{
				fprintf(stderr, "inode_indexlookup(blockNum=%d) retornando -1 no diskimage_readsector ponteiro 1 nivel \n",
						blockNum) ;
				return -1 ;
			}

			return bufferPonteiros[indexPonteiroDireto] ;
		}
	}
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
