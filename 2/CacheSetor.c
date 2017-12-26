#include "CacheSetor.h"

#include <openssl/lhash.h>
#include <string.h>
#include "cachemem.h"
#include "utilidades/list.h"
#include "assign1/diskimg.h"

/*
 * struct privada ao módulo
 */

typedef struct
{
	int numeroSetor ;
	struct list_head l ;

} DescritorSetorCache ;


static void *inicioCache ;
static int quantSetoresCache ;
static int tamanhoBytes ;

static DescritorSetorCache *descritores ;

static LIST_HEAD( listaLivres );

static LIST_HEAD( listaLRU );

static _LHASH *hashTable ;


static unsigned long HashCallBack(const void *arg)
{
	/*
	 * a chave da hash table é o numero do setor do disco
	 */

	return ((DescritorSetorCache*) arg)->numeroSetor ;
}


static int CompareCallBack(const void *arg1, const void *arg2)
{
	return ((DescritorSetorCache*) arg1)->numeroSetor - ((DescritorSetorCache*) arg2)->numeroSetor ;
}


void CacheSetor_init(void* inicio, int tamanho)
{
	inicioCache = inicio ;
	tamanhoBytes = tamanho ;
	quantSetoresCache = tamanhoBytes / DISKIMG_SECTOR_SIZE ;

	descritores = malloc(sizeof(DescritorSetorCache) * quantSetoresCache) ;

	for (int i = 0; i < quantSetoresCache; i++)
	{
		INIT_LIST_HEAD( &descritores[i].l )
		;
		list_add_tail(&descritores[i].l, &listaLivres) ;
	}

	hashTable = lh_new(HashCallBack, CompareCallBack) ;
}


static void *descritorSetorToEnderecoSetor(DescritorSetorCache *desc)
{
	return inicioCache + (desc - descritores) * DISKIMG_SECTOR_SIZE ;
}


void *CacheSetor_obterSetor(int numeroSetor)
{
	DescritorSetorCache *desc = NULL ;

	DescritorSetorCache temp ;
	temp.numeroSetor = numeroSetor ;

	desc = lh_retrieve(hashTable, &temp) ;

	if (desc == NULL)
		return NULL ;

	list_del_init(&desc->l) ;
	list_add(&desc->l, &listaLRU) ;

	return descritorSetorToEnderecoSetor(desc) ;
}


void CacheSetor_adicionarSetor(int numeroSetor, void *setor)
{
	if (!list_empty(&listaLivres))
	{
		DescritorSetorCache *desc = list_entry(listaLivres.next, DescritorSetorCache, l) ;
		list_del_init(&desc->l) ;

		memcpy(descritorSetorToEnderecoSetor(desc), setor, DISKIMG_SECTOR_SIZE) ;
		desc->numeroSetor = numeroSetor ;

		lh_insert(hashTable, desc) ;

		list_add(&desc->l, &listaLRU) ;
	}
	else
	{
		/*
		 * nao possui espaço livre no cache, portanto pega o fim da lista LRU para a nova entrada.
		 * retira da hashTable pois a chave vai mudar. Adiciona o descritor com a nova chave na hashTable.
		 */

		DescritorSetorCache *desc = list_entry(listaLRU.prev, DescritorSetorCache, l) ;
		list_del_init(&desc->l) ;

		lh_delete(hashTable, desc) ;

		memcpy(descritorSetorToEnderecoSetor(desc), setor, DISKIMG_SECTOR_SIZE) ;
		desc->numeroSetor = numeroSetor ;

		lh_insert(hashTable, desc) ;

		list_add(&desc->l, &listaLRU) ;
	}
}
