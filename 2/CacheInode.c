#include "CacheInode.h"

#include "assign1/ino.h"
#include "cachemem.h"
#include "utilidades/list.h"
#include <openssl/lhash.h>
#include "assign1/diskimg.h"
#include <string.h>

/*
 * struct privada a esse módulo por isso não está no header
 */

typedef struct
{
	int inumber ;
	struct list_head l ;

} DescritorInodeCache ;


static void *inicioCache ;
static int tamanhoBytes ;
static int quantInodesCache ;

static DescritorInodeCache *descritores ;

static LIST_HEAD(listaLivres);
static LIST_HEAD(listaLRU);

static _LHASH *hashTable ;


static unsigned long HashCallBack(const void *arg)
{
	/*
	 * chave da hash table é o inode number
	 */

	return ((DescritorInodeCache*) arg)->inumber ;
}


static int CompareCallBack(const void *arg1, const void *arg2)
{
	return ((DescritorInodeCache*) arg1)->inumber - ((DescritorInodeCache*) arg2)->inumber ;
}


void CacheInode_init(void *inicio, int tamanho)
{
	inicioCache = inicio ;
	tamanhoBytes = tamanho ;
	quantInodesCache = (tamanho / DISKIMG_SECTOR_SIZE) * INODES_POR_BLOCO ;

	descritores = malloc(sizeof(DescritorInodeCache) * quantInodesCache) ;

	for (int i = 0; i < quantInodesCache; i++)
	{
		INIT_LIST_HEAD(&descritores[i].l) ;
		list_add(&descritores[i].l, &listaLivres) ;
	}

	hashTable = lh_new(HashCallBack, CompareCallBack) ;
}


static void *descritorInodeToEnderecoInode(DescritorInodeCache *desc)
{
	return inicioCache + (desc - descritores) * sizeof(struct inode) ;
}


struct inode *CacheInode_obterInode(int inumber)
{
	DescritorInodeCache *desc = NULL ;

	DescritorInodeCache temp ;
	temp.inumber = inumber ;

	desc = lh_retrieve(hashTable, &temp) ;

	if (desc == NULL)
		return NULL ;

	list_del_init(&desc->l) ;
	list_add(&desc->l, &listaLRU) ;

	return descritorInodeToEnderecoInode(desc) ;
}


void CacheInode_adicionarInode(int inumber, struct inode *ind)
{
	if (!list_empty(&listaLivres))
	{
		DescritorInodeCache *desc = list_entry(listaLivres.next, DescritorInodeCache, l) ;
		list_del_init(&desc->l) ;

		memcpy(descritorInodeToEnderecoInode(desc), ind, sizeof(struct inode)) ;
		desc->inumber = inumber ;

		lh_insert(hashTable, desc) ;

		list_add(&desc->l, &listaLRU) ;
	}
	else
	{
		/*
		 * nao possui espaço livre no cache, portanto pega o fim da lista LRU para a nova entrada.
		 * retira da hashTable pois a chave vai mudar. Adiciona o descritor com a nova chave na hashTable.
		 */

		DescritorInodeCache *desc = list_entry(listaLRU.prev, DescritorInodeCache, l) ;
		list_del_init(&desc->l) ;

		lh_delete(hashTable, desc) ;

		memcpy(descritorInodeToEnderecoInode(desc), ind, sizeof(struct inode)) ;
		desc->inumber = inumber ;

		lh_insert(hashTable, desc) ;

		list_add(&desc->l, &listaLRU) ;
	}
}
