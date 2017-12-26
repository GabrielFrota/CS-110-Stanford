/**
 * File: http-proxy-scheduler.h
 * ----------------------------
 * This class defines the scheduler class, which takes all
 * proxied requests off of the main thread and schedules them to 
 * be handled by a constant number of child threads.
 */

#ifndef _http_proxy_scheduler_
#define _http_proxy_scheduler_

#include <string>
#include "request-handler.h"
#include "thread-pool.h"


class HTTPProxyScheduler {

 public:

	HTTPProxyScheduler(unsigned short *portNumber, std::string **proxyServer, unsigned short *proxyServerPort)
 	 : requestPool(64), handler(portNumber, proxyServer, proxyServerPort)
	{
		this->proxyServer = proxyServer ;
		this->proxyServerPort = proxyServerPort ;
	}

    void scheduleRequest(int connectionfd, const std::string& clientIPAddress, pthread_mutex_t *mutexCon);

	HTTPRequestHandler getHTTPRequestHandler()
	{
		return handler ;
	}

 private:

  HTTPRequestHandler handler;
  ThreadPool requestPool;
  std::string **proxyServer ;
  unsigned short *proxyServerPort ;

};

#endif
