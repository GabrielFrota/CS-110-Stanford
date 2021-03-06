/**
 * File: cache.h
 * -------------
 * Defines a class to help manage an 
 * HTTP response cache.
 */

#ifndef _http_cache_
#define _http_cache_

#include <string>
#include <memory>
#include "request.h"
#include "response.h"
#include <pthread.h>

class HTTPCache {
 public:

/**
 * Constructs the HTTPCache object.
 */

  HTTPCache( unsigned short *portNumber, std::string **proxyServer, unsigned short *proxyServerPort ) ;

/**
 * The following three functions do what you'd expect, except that they 
 * aren't thread safe.  In a MT environment, you should acquire the lock
 * on the relevant response before calling these.
 */
  bool containsCacheEntry(const HTTPRequest& request, HTTPResponse& response) const;
  bool shouldCache(const HTTPRequest& request, const HTTPResponse& response) const;
  void cacheEntry(const HTTPRequest& request, const HTTPResponse& response);
  
  void lockMutex( const HTTPRequest& request ) ;

  void unlockMutex( const HTTPRequest& request ) ;

  void atualizarStringDiretorio() ;

 private:
  std::string hashRequest(const HTTPRequest& request) const;
  std::string serializeRequest(const HTTPRequest& request) const;
  bool cacheEntryExists(const std::string& filename) const;
  std::string getRequestHashCacheEntryName(const std::string& requestHash) const;
  void ensureDirectoryExists(const std::string& directory, bool empty = false) const;
  std::string getExpirationTime(int ttl) const;
  bool cachedEntryIsValid(const std::string& cachedFileName) const;
  
  std::string cacheDirectory;
  pthread_mutex_t mutexes[997] ;

  unsigned short *portNumber ;
  std::string **proxyServer ;
  unsigned short *proxyServerPort ;

};

#endif
