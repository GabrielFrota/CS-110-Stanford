/**
 * File: scheduler.cc
 * ------------------
 * Presents the implementation of the HTTPProxyScheduler class.
 */

#include "scheduler.h"
#include "request-handler.h"
#include <utility>      // for make_pair
using namespace std;


void HTTPProxyScheduler::scheduleRequest(int connectionfd, const string& clientIPAddress, pthread_mutex_t *mutexSocket)
{
	Parametros *p = new Parametros ;

	p->handler = &handler ;
	p->connection = make_pair(connectionfd, clientIPAddress) ;
	p->mutexSocket = mutexSocket ;

	if (*proxyServer != NULL)
	{
		p->proxyServer = *proxyServer ;
		p->proxyServerPort = *proxyServerPort ;

	}
	else
	{
		p->proxyServer = NULL ;
		p->proxyServerPort = 0 ;
	}

	Funcao f ;

	f.f = handler.getServiceRequestPtr() ;
	f.param = (void*) p ;

	requestPool.schedule(f) ;
}
