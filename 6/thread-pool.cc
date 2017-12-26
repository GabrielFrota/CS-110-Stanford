/* File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
#include <iostream>
#include <functional>
#include <queue>


static void *dispatcher(void *param) ;
static void *trabalhar(void *param) ;
static void novaThreadTrabalhar(ThreadDispatcher *threadD, ThreadTrabalhar *threadT) ;


static void *dispatcher(void *param)
{
	ThreadDispatcher *td = (ThreadDispatcher*) param ;

	while (1)
	{
		Funcao funcaoParaExecutar ;
		ThreadTrabalhar *threadEscolhida ;

		pthread_mutex_lock(&td->mutexFilaFuncoes) ;

		while (!td->flagExitDispatcher && td->filaFuncoes->empty())
		{
			pthread_cond_wait(&td->condNovaFuncaoNaFila, &td->mutexFilaFuncoes) ;
		}

		if (td->flagExitDispatcher && td->filaFuncoes->empty())
		{
			pthread_mutex_unlock(&td->mutexFilaFuncoes) ;
			td->flagExitTrabalhar = 1 ;

			pthread_exit(0) ;
		}

		funcaoParaExecutar = td->filaFuncoes->front() ;
		td->filaFuncoes->pop() ;

		pthread_mutex_unlock(&td->mutexFilaFuncoes) ;

		pthread_mutex_lock(&td->mutexFilaThreads) ;

		while (td->filaThreads->empty())
		{
			if (td->quantThreads < td->quantThreadsMax)
			{
				novaThreadTrabalhar(td, (td->threadsT) + (td->quantThreads)) ;

				td->quantThreads++ ;

			}
			else
			{
				pthread_cond_wait(&td->condNovaThreadNaFila, &td->mutexFilaThreads) ;
			}
		}

		threadEscolhida = td->filaThreads->front() ;
		td->filaThreads->pop() ;

		pthread_mutex_unlock(&td->mutexFilaThreads) ;

		pthread_mutex_lock(&threadEscolhida->mutex) ;

		threadEscolhida->funcaoTrabalhar = funcaoParaExecutar ;
		pthread_cond_signal(&threadEscolhida->condContinuar) ;

		pthread_mutex_unlock(&threadEscolhida->mutex) ;
	}

	return 0 ;
}


static void novaThreadTrabalhar(ThreadDispatcher *threadD, ThreadTrabalhar *threadT)
{
	threadT->filaThreads = threadD->filaThreads ;
	threadT->mutexFilaThreads = &threadD->mutexFilaThreads ;
	threadT->condNovaThreadNaFila = &threadD->condNovaThreadNaFila ;

	pthread_mutex_init(&threadT->mutex, NULL) ;
	pthread_cond_init(&threadT->condContinuar, NULL) ;
	threadT->funcaoTrabalhar.f = NULL ;
	threadT->funcaoTrabalhar.param = NULL ;

	threadT->flagExit = &threadD->flagExitTrabalhar ;

	pthread_create(&threadT->td, NULL, trabalhar, threadT) ;
}


static void *trabalhar(void *param)
{
	ThreadTrabalhar *tdT = (ThreadTrabalhar*) param ;

	while (1)
	{
		pthread_mutex_lock(tdT->mutexFilaThreads) ;

		tdT->filaThreads->push(tdT) ;
		pthread_cond_signal(tdT->condNovaThreadNaFila) ;

		pthread_mutex_unlock(tdT->mutexFilaThreads) ;

		pthread_mutex_lock(&tdT->mutex) ;

		while (!(*tdT->flagExit) && tdT->funcaoTrabalhar.f == NULL)
		{
			pthread_cond_wait(&tdT->condContinuar, &tdT->mutex) ;
		}

		if (*tdT->flagExit && tdT->funcaoTrabalhar.f == NULL)
		{
			pthread_mutex_unlock(&tdT->mutex) ;
			pthread_exit(0) ;
		}

		pthread_mutex_unlock(&tdT->mutex) ;

		/*
		 * Quando flagExit=1 é garantido que não existem mais funçoes na fila, mas pode ser
		 * que ainda existam threads de trabalho com uma função para executar. Será a ultima iteração dai, pois na
		 * proxima funcaoTrabalhar=NULL
		 */

		tdT->funcaoTrabalhar.f(tdT->funcaoTrabalhar.param) ;

		tdT->funcaoTrabalhar.f = NULL ;
		tdT->funcaoTrabalhar.param = NULL ;
	}

	return 0 ;
}


ThreadPool::ThreadPool(size_t numThreads)
{
	threadsT = new ThreadTrabalhar[numThreads] ;

	threadD.filaFuncoes = new std::queue<Funcao> ;
	pthread_mutex_init(&threadD.mutexFilaFuncoes, NULL) ;
	pthread_cond_init(&threadD.condNovaFuncaoNaFila, NULL) ;
	pthread_cond_init(&threadD.condFilaFuncoesVazia, NULL) ;

	threadD.filaThreads = new std::queue<ThreadTrabalhar*> ;
	pthread_mutex_init(&threadD.mutexFilaThreads, NULL) ;
	pthread_cond_init(&threadD.condNovaThreadNaFila, NULL) ;

	threadD.flagExitDispatcher = 0 ;
	threadD.flagExitTrabalhar = 0 ;

	threadD.threadsT = threadsT ;
	threadD.quantThreads = 0 ;
	threadD.quantThreadsMax = numThreads ;

	pthread_create(&threadD.td, NULL, dispatcher, &threadD) ;
}


ThreadPool::~ThreadPool()
{
	delete (threadD.filaFuncoes) ;
	delete (threadD.filaThreads) ;
	delete[] threadsT ;
}


void ThreadPool::schedule(Funcao f)
{
	pthread_mutex_lock(&threadD.mutexFilaFuncoes) ;

	threadD.filaFuncoes->push(f) ;
	pthread_cond_signal(&threadD.condNovaFuncaoNaFila) ;

	pthread_mutex_unlock(&threadD.mutexFilaFuncoes) ;
}


void ThreadPool::wait()
{
	pthread_mutex_lock(&threadD.mutexFilaFuncoes) ;

	threadD.flagExitDispatcher = 1 ;
	pthread_cond_signal(&threadD.condNovaFuncaoNaFila) ;

	pthread_mutex_unlock(&threadD.mutexFilaFuncoes) ;

	pthread_join(threadD.td, NULL) ;

	for (int i = 0; i < threadD.quantThreadsMax; i++)
	{
		pthread_mutex_lock(&threadsT[i].mutex) ;

		pthread_cond_signal(&threadsT[i].condContinuar) ;

		pthread_mutex_unlock(&threadsT[i].mutex) ;

		pthread_join(threadsT[i].td, NULL) ;
	}
}
