
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

int pathname_lookup(struct unixfilesystem *fs, const char *pathname)
{
	char *token, *pathnameTemp ;

	pathnameTemp = strdup(pathname) ;

	int inodeAtual = ROOT_INUMBER ;
	struct direntv6 diretorioAtual ;
	token = strtok(pathnameTemp, "/") ;

	while (token != NULL)
	{
		directory_findname(fs, token, inodeAtual, &diretorioAtual) ;
		inodeAtual = diretorioAtual.d_inumber ;
		token = strtok(NULL, "/") ;
	}

	free(pathnameTemp) ;

	return inodeAtual ;
}
