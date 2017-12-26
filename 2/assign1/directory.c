#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

// remove the placeholder implementation and replace with your own
int directory_findname(struct unixfilesystem *fs, const char *name,
					   int dirinumber, struct direntv6 *dirEnt)
{
	struct inode ind ;
	struct direntv6 *entradasDiretorio ;
	int quantBlocos ;

	inode_iget(fs, dirinumber, &ind) ;

	int tamanhoArquivo = inode_getsize(&ind) ;
	int quantEntradasDiretorio = tamanhoArquivo / sizeof(struct direntv6) ;

	if ((ind.i_mode & IFDIR) == 0)
	{
		printf("\nNAO DIRETORIO\n") ;
		return -1 ;
	}

	if (tamanhoArquivo % DISKIMG_SECTOR_SIZE != 0)
	{
		quantBlocos = tamanhoArquivo / DISKIMG_SECTOR_SIZE + 1 ;
		entradasDiretorio = malloc(DISKIMG_SECTOR_SIZE * quantBlocos) ;
	}
	else
	{
		quantBlocos = tamanhoArquivo / DISKIMG_SECTOR_SIZE ;
		entradasDiretorio = malloc(DISKIMG_SECTOR_SIZE * quantBlocos) ;
	}

	for (int i = 0; i < quantBlocos; i++)
	{
		file_getblock(fs, dirinumber, i, (char*) entradasDiretorio + i * DISKIMG_SECTOR_SIZE) ;
	}

	struct direntv6 *resultadoBusca = NULL ;

	for (int i = 0; i < quantEntradasDiretorio; i++)
	{
		if (strcmp(name, entradasDiretorio[i].d_name) == 0)
		{
			resultadoBusca = entradasDiretorio + i ;
			break ;
		}
	}

	if (resultadoBusca != NULL)
	{
		*dirEnt = *resultadoBusca ;
		return 0 ;
	}

	return -1 ;
}
