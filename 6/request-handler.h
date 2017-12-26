/**
 * File: request-handler.h
 * -----------------------
 * Defines the HTTPRequestHandler class, which proxies and
 * services a single client request.  
 */

#ifndef _http_request_handler_
#define _http_request_handler_

#include <utility>     // for pair
#include <string>      // for string

#include "blacklist.h"
#include "cache.h"
#include <socket++/sockstream.h>

/*
 * typedef para declarar uma funçao que retorna o endereço da funçao serviceRequest, que é local ao arquivo request-handler.cc,
 * pois é static.
 */

typedef void* (*voidFp)(void*) ;


class HTTPRequestHandler {

 public:

	HTTPRequestHandler(unsigned short *portNumber, std::string **proxyServer, unsigned short *proxyServerPort)
	: blacklist("blocked-domains.txt"), cache(portNumber, proxyServer, proxyServerPort) {} ;

	HTTPBlacklist blacklist ;
	HTTPCache cache ;

	static void escreverResponseAcessoNegado( iosockstream &stream ) ;

	voidFp getServiceRequestPtr() ;

 private:

};


/*
 * parametros que serao passados para a funçao no pthread_create
 */

struct Parametros
{
	HTTPRequestHandler *handler ;
	std::pair<int, std::string> connection ;
	std::pair<int, pthread_mutex_t> connectionLock ;
	std::string *proxyServer ;
	unsigned short proxyServerPort ;
	pthread_mutex_t *mutexSocket ;
} ;


#endif
