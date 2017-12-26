/**
 * File: request-handler.cc
 * ------------------------
 * Provides the implementation for the HTTPRequestHandler class.
 */

#include "request-handler.h"
#include "response.h"
#include "request.h"
#include "scheduler.h"
#include "PrintLock.h"

#include <iostream>              // for flush
#include <string>                // for string
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <getopt.h>
#include <unistd.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>


using namespace std;


static void escreverResponseAcessoNegado(iosockstream &stream)
{
	HTTPResponse response ;

	response.setResponseCode(403) ;
	response.setPayload("Forbidden Content") ;

	stream << response << flush ;
}


static void escreverResponseCicloProxy(iosockstream &stream)
{
	HTTPResponse response ;

	response.setResponseCode(504) ;
	response.setPayload("Ciclo de proxies detectado") ;

	stream << response << flush ;
}


static void imprimirErro(const string &msg, const string &valor)
{
	PrintLock::lock() ;

	cerr << msg << valor << endl ;

	PrintLock::unlock() ;
}


static void imprimirEscritaFalhou(const string &urlRequest, const string &urlCliente, const string &porta)
{
	PrintLock::lock() ;

	cerr << "Escrita da response do request " << urlRequest <<
			" no socket do cliente " << urlCliente << ":" << porta << " falhou." << endl ;

	PrintLock::unlock() ;
}


/**
 * Reads the entire HTTP request from the provided socket (the int portion
 * of the connection pair), passes it on to the origin server,
 * waits for the response, and then passes that response back to
 * the client (whose IP address is given by the string portion
 * of the connection pair.
 */

static void* serviceRequest(void *param)
{
	Parametros *p = (Parametros*) param ;

	struct sockaddr clienteAddr, hostAddr ;
	socklen_t tamSockAddr = sizeof(sockaddr) ;
	char clienteAddrTexto[INET_ADDRSTRLEN], hostAddrTexto[INET_ADDRSTRLEN] ;
	sockaddr_in *clienteAddrPtr = (sockaddr_in*)&clienteAddr ;
	sockaddr_in *hostAddrPtr = (sockaddr_in*)&hostAddr ;

	sockbuf *bufferCliente = new sockbuf(p->connection.first) ;
	iosockstream *streamCliente = new iosockstream(bufferCliente) ;

	HTTPRequest requestCliente ;
	HTTPResponse responseEmCache ;

	if (getpeername(p->connection.first, &clienteAddr, &tamSockAddr) == -1)
	{
		imprimirErro("Chamada getpeername na funçao "
				"HTTPRequestHandler::serviceRequest retornou erro ", to_string(errno)) ;

		goto fimFuncao ;
	}

	inet_ntop(AF_INET, &clienteAddrPtr->sin_addr, clienteAddrTexto, INET_ADDRSTRLEN) ;

	/*
	 * processa a requisiçao interceptada
	 */

	try
	{
		pthread_mutex_lock(p->mutexSocket) ;

		requestCliente.ingestRequestLine(*streamCliente) ;
		requestCliente.ingestHeader(*streamCliente, p->connection.second) ;
		requestCliente.ingestPayload(*streamCliente) ;

		pthread_mutex_unlock(p->mutexSocket) ;
	}
	catch (HTTPBadRequestException &e)
	{
		pthread_mutex_unlock(p->mutexSocket) ;

		imprimirErro("HTTPBadRequestException na chamada HTTPRequest::ingestRequestLine do socket ",
				to_string(p->connection.first)) ;

		goto fimFuncao ;
	}

	if (p->proxyServer != NULL)
	{
		/*
		 * sou um proxy que redireciona para outro proxy. aqui sera feita a montagem da string
		 * que será o valor da entrada "x-forwarded-for" no cabeçalho,
		 * que indica a sequencia de proxies que o request passou.
		 */

		requestCliente.adicionarHeaderEntity("x-forwarded-proto", "http") ;

		if (requestCliente.containsName("x-forwarded-for"))
		{
			/*
			 * checar se existe ciclo de proxies. "x-forwarded-for" é uma sequencia de sockaddr_in.sin_addr,
			 * portanto pega o sockaddr_in.sin_addr do proxy atual, e compara-o com a sequencia
			 * de proxies que o pacote passou.
			 */

			if (getsockname(p->connection.first, &hostAddr, &tamSockAddr) == -1)
			{
				imprimirErro("Chamada getsockname na funçao HTTPRequestHandler::serviceRequest retornou erro ", to_string(errno)) ;

				goto fimFuncao ;
			}

			inet_ntop( AF_INET, &hostAddrPtr->sin_addr, hostAddrTexto, INET_ADDRSTRLEN) ;

			string s(requestCliente.getValorHeaderComoString("x-forwarded-for")) ;

			string stringHost(hostAddrTexto) ;
			stringHost.append(":") ;
			stringHost.append(to_string(hostAddrPtr->sin_port)) ;

			vector<string> stringsNoHeader ;
			boost::split(stringsNoHeader, s, boost::is_any_of(","), boost::token_compress_off) ;

			for (unsigned int i = 0; i < stringsNoHeader.size(); i++)
			{
				if (strcmp(stringsNoHeader[i].c_str(), stringHost.c_str()) == 0)
				{
					pthread_mutex_lock(p->mutexSocket) ;

					escreverResponseCicloProxy(*streamCliente) ;
					bool flagFalhou = streamCliente->fail() ;

					pthread_mutex_unlock(p->mutexSocket) ;

					if (flagFalhou)
					{
						imprimirEscritaFalhou(requestCliente.getURL(), p->connection.second,
								to_string(clienteAddrPtr->sin_port)) ;
					}

					goto fimFuncao ;
				}
			}

			/*
			 * ciclo nao encontrado. Concatenar na string "x-forwarded-for" o clienteAddr atual.
			 */

			s.append(",") ;
			s.append(clienteAddrTexto) ;
			s.append(":") ;
			s.append(std::to_string(clienteAddrPtr->sin_port)) ;
			requestCliente.adicionarHeaderEntity("x-forwarded-for", s) ;
		}
		else
		{
			/*
			 * sou o primeiro proxy que a requisiçao passa.
			 */

			string s(clienteAddrTexto, INET_ADDRSTRLEN) ;
			s.append(":") ;
			s.append(std::to_string(clienteAddrPtr->sin_port)) ;
			requestCliente.adicionarHeaderEntity("x-forwarded-for", s) ;
		}
	}

	/*
	 * checa se URL do destino da requisição não é um endereço bloqueado
	 */

	if (!p->handler->blacklist.serverIsAllowed(requestCliente.getServer()))
	{
		pthread_mutex_lock(p->mutexSocket) ;

		escreverResponseAcessoNegado(*streamCliente) ;
		bool flagFalhou = streamCliente->fail() ;

		pthread_mutex_unlock(p->mutexSocket) ;

		if (flagFalhou)
		{
			imprimirEscritaFalhou(requestCliente.getURL(), p->connection.second,
					to_string(clienteAddrPtr->sin_port)) ;
		}

		goto fimFuncao ;
	}

	/*
	 * checa se a resposta da requisiçao esta em cache
	 */

	p->handler->cache.lockMutex(requestCliente) ;

	if (p->handler->cache.containsCacheEntry(requestCliente, responseEmCache))
	{
		pthread_mutex_lock(p->mutexSocket) ;

		*streamCliente << responseEmCache << flush ;

		pthread_mutex_unlock(p->mutexSocket) ;

		goto unlockMutexCache ;
	}

	/*
	 * inicia uma conexao com o destino da requisiçao interceptada. O destino pode ser a URL original da requisição,
	 * ou um outro proxy. Depende da configuração deste proxy.
	 */

	struct addrinfo *res ;

	{
		int socketDestino ;
		struct addrinfo hints ;

		socketDestino = socket( AF_INET, SOCK_STREAM, 0) ;

		if (socketDestino == -1)
		{
			imprimirErro("Chamada socket na funçao HTTPRequestHandler::serviceRequest retornou erro ", to_string(errno)) ;

			goto unlockMutexCache ;
		}

		const int optval = 1 ;

		if (setsockopt(socketDestino, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
		{
			imprimirErro("Chamada setsockopt na funçao HTTPRequestHandler::serviceRequest retornou erro ", to_string(errno)) ;

			goto unlockMutexCache ;
		}

		bzero(&hints, sizeof(hints)) ;

		hints.ai_family = AF_INET ;
		hints.ai_socktype = SOCK_STREAM ;

		if (p->proxyServer != NULL)
		{
			/*
			 * sou um proxy que redireciona para outro proxy. conexão será com um outro proxy, e não com o destino original da requisiçao
			 */

			if (getaddrinfo(p->proxyServer->c_str(), to_string(p->proxyServerPort).c_str(), &hints, &res) != 0)
			{
				imprimirErro("Chamada getaddrinfo proxyServer "
						"na funçao HTTPRequestHandler::serviceRequest retornou erro ", to_string(errno)) ;

				goto unlockMutexCache ;
			}
		}
		else
		{
			/*
			 * sou um proxy que repassa a requisição para o destino original do cliente
			 */

			if (getaddrinfo(requestCliente.getServer().c_str(), "http", &hints, &res) != 0)
			{
				imprimirErro("Chamada getaddrinfo na funçao "
						"HTTPRequestHandler::serviceRequest retornou erro ", to_string(errno)) ;

				goto unlockMutexCache ;
			}
		}

		if (connect(socketDestino, res->ai_addr, res->ai_addrlen) == -1)
		{
			imprimirErro("Chamada connect na funçao "
					"HTTPRequestHandler::serviceRequest retornou erro ", to_string(errno)) ;

			goto freeAddrInfo ;
		}

		/*
		 * escreve a requisiçao interceptada no stream entre o proxy e o destino
		 */

		sockbuf bufferDestino(socketDestino) ;
		iosockstream streamDestino(&bufferDestino) ;

		if (p->proxyServer != NULL)
		{
			/*
			 * requisicao sera mandadada para outro proxy, portanto escreve o request inteiro, sem usar
			 * o operador << que foi alterado para escrever o cabeçalho diferente, removendo algumas partes.
			 */

			requestCliente.escreverRequestCompleto(streamDestino) ;
		}
		else
		{
			/*
			 * requisicao sera mandadada para a URL original. Escreve o cabeçalho com o operador <<, e antes
			 * retira as 2 entradas que foram adicionadas pelos proxies, pois alguns sites retornam erro "Bad Request"
			 * se enviar o request com essas entradas.
			 */

			requestCliente.getRequestHeader().removeHeader("x-forwarded-for") ;
			requestCliente.getRequestHeader().removeHeader("x-forwarded-proto") ;

			streamDestino << requestCliente << flush ;
		}

		/*
		 * processa a response do destino
		 */

		HTTPResponse responseDestino ;

		responseDestino.ingestResponseHeader(streamDestino) ;
		responseDestino.ingestPayload(streamDestino) ;

		/*
		 * salva a response no cache caso seja possivel
		 */

		if (p->handler->cache.shouldCache(requestCliente, responseDestino))
		{
			p->handler->cache.cacheEntry(requestCliente, responseDestino) ;
		}

		/*
		 * escreve a response do destino no socket entre o cliente e o proxy
		 */

		pthread_mutex_lock(p->mutexSocket) ;

		*streamCliente << responseDestino << flush ;
		bool flagFalhou = streamCliente->fail() ;

		pthread_mutex_unlock(p->mutexSocket) ;

		if (flagFalhou)
		{
			imprimirEscritaFalhou(requestCliente.getURL(), p->connection.second,
					to_string(clienteAddrPtr->sin_port)) ;
		}
	}

	/*
	 * devolve recursos
	 */

	freeAddrInfo:
	freeaddrinfo(res) ;

	unlockMutexCache:
	p->handler->cache.unlockMutex(requestCliente) ;

	fimFuncao:
	pthread_mutex_lock(p->mutexSocket) ;

	if (streamCliente->fail())
	{
		try
		{
			bufferCliente->shutdown(sockbuf::shut_write) ;
		}
		catch (sockerr &se)
		{
		}
	}

	try
	{
		delete (bufferCliente) ;
	}
	catch (sockerr &se)
	{
	}

	pthread_mutex_unlock(p->mutexSocket) ;

	delete (streamCliente) ;

	free(param) ;
}

voidFp HTTPRequestHandler::getServiceRequestPtr()
{
	return serviceRequest ;
}

