/**
 * File: news-aggregator.cc
 * ------------------------
 * When fully implemented, news-aggregator.cc pulls, parses,
 * and indexes every single news article reachable from some
 * RSS feed in the user-supplied RSS News Feed XML file, 
 * and then allows the user to query the index.
 *
 *
 * small-feed.xml possui apenas feeds válidos.
 *
 * large-feed.xml possui varios feeds invalidos ou que darão problemas, aqui as checagens de erros sao testadas.
 *
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <libxml/parser.h>
#include <libxml/catalog.h>
#include <getopt.h>

#include <thread>
#include <mutex>
#include <condition_variable>

#include <pthread.h>
#include <semaphore.h>

#include "article.h"
#include "rss-feed-list.h"
#include "rss-feed.h"
#include "rss-index.h"
#include "html-document.h"
#include "html-document-exception.h"
#include "rss-feed-exception.h"
#include "rss-feed-list-exception.h"
#include "news-aggregator-utils.h"
#include "string-utils.h"
using namespace std;

// globals
static bool verbose = false;
static RSSIndex index;

static map<string, sem_t*> servidorConexoes ;
static sem_t sem_feedsDown, sem_artigosDown ;

static pthread_mutex_t mut_print, mut_index ;


static const int kIncorrectUsage = 1;
static void printUsage(const string& message, const string& executableName) {
  cerr << "Error: " << message << endl;
  cerr << "Usage: " << executableName << " [--verbose] [--quiet] [--url <feed-file>]" << endl;
  exit(kIncorrectUsage);
}

static const string kDefaultRSSFeedListURL = "small-feed.xml";
static string parseArgumentList(int argc, char *argv[]) {
  struct option options[] = {
    {"verbose", no_argument, NULL, 'v'},
    {"quiet", no_argument, NULL, 'q'},
    {"url", required_argument, NULL, 'u'},
    {NULL, 0, NULL, 0},
  };

  string url = kDefaultRSSFeedListURL;
  while (true) {
    int ch = getopt_long(argc, argv, "vqu:", options, NULL);
    if (ch == -1) break;
    switch (ch) {
    case 'v':
      verbose = true;
      break;
    case 'q':
      verbose = false;
      break;
    case 'u':
      url = optarg;
      break;
    default:
      printUsage("Unrecognized flag.", argv[0]);
    }
  }
  
  argc -= optind;
  argv += optind;
  if (argc > 0) {
    printUsage("Too many arguments", argv[0]);
  }
  return url;
}

static void *processarFeed(void *url);


static void *processarArtigo(void *art);


static const int kBogusRSSFeedListName = 1;
static void processAllFeeds(const string& feedListURI)
{
	RSSFeedList feedList(feedListURI) ;
	try
	{
		feedList.parse() ;
	}
	catch (const RSSFeedListException& rfle)
	{
		cerr << "Ran into trouble while pulling full RSS feed list from \""
				<< feedListURI << "\"." << endl ;
		cerr << "Aborting...." << endl ;
		exit(kBogusRSSFeedListName) ;
	}

	// note to student
	// ---------------
	// add well-decomposed code to read all of the RSS
	// news feeds from feedList for their news articles,
	// and for each news article URL, process it
	// as an HTMLDocument and add all of the tokens
	// to the master RSSIndex.

	sem_init(&sem_feedsDown, 0, 10) ;
	sem_init(&sem_artigosDown, 0, 32) ;

	pthread_mutex_init(&mut_print, NULL) ;
	pthread_mutex_init(&mut_index, NULL) ;

	map<string, string>::iterator iteradorFeed ;
	auto feeds = feedList.getFeeds() ;

	pthread_t *threads = new pthread_t[feeds.size()] ;
	unsigned i = 0 ;

	for (iteradorFeed = feeds.begin() ; iteradorFeed != feeds.end(); iteradorFeed++)
	{
		sem_wait(&sem_feedsDown) ;

		pthread_mutex_lock(&mut_print) ;
		cout << "Criando thread processarFeed " << iteradorFeed->first << endl ;
		pthread_mutex_unlock(&mut_print) ;

		pthread_create(threads + i, NULL, processarFeed, (void*) &(iteradorFeed->first)) ;
		i++ ;
	}

	for (i = 0 ; i < feeds.size(); i++)
	{
		pthread_join(threads[i], NULL) ;
	}

	delete (threads) ;

	sem_destroy(&sem_feedsDown) ;
	sem_destroy(&sem_artigosDown) ;

	pthread_mutex_destroy(&mut_print) ;
	pthread_mutex_destroy(&mut_index) ;
}


static void *processarFeed(void *url)
{
	string &strUrl = *(string*) url ;

	RSSFeed feed(strUrl) ;

	try
	{
		feed.parse() ;
	}
	catch (RSSFeedException &rssEx)
	{
		/*
		 * URL informada no arquivo xml pode ser invalida, ou a pagina possui elementos que a biblioteca nao consegue processar.
		 */

		pthread_mutex_lock(&mut_print) ;
		cerr << "Problema ao processar feed " << strUrl << " , abortando processamento e finalizando thread." << endl ;
		pthread_mutex_unlock(&mut_print) ;

		goto finalizacao ;
	}

	{
		auto artigos = feed.getArticles() ;

		if (artigos.size() == 0)
		{
			/*
			 *  A biblioteca usada é uma biblioteca C, e a camada que joga exceçoes é a camada C++ fornecida no trabalho.
			 *  As vezes nao acontece exceçao na camada C++, mas a biblioteca retorna uma lista de artigos vazia pois algum problema
			 *  aconteceu.
			 */

			pthread_mutex_lock(&mut_print) ;
			cerr << "Lista de artigos do feed " << strUrl << " invalida, abortando processamento e finalizando thread." << endl ;
			pthread_mutex_unlock(&mut_print) ;

			goto finalizacao ;
		}

		/*
		 *  Obtendo a url do servidor pela url dos artigos, pois as vezes a url fornecido no arquivo xml gera um redirect para outra
		 *  url com um prefixo diferente, e isso causa problemas pois o prefixo do servidor da url do xml e da url dos artigos vai
		 *  ser diferente.
		 */

		string strServidor = getURLServer(artigos.begin()->url) ;

		sem_t *sem_conexoes = new (sem_t) ;
		sem_init(sem_conexoes, 0, 6) ;

		servidorConexoes.insert(pair<string, sem_t*>(strServidor, sem_conexoes)) ;

		pthread_t *threads = new pthread_t[artigos.size()] ;
		unsigned i = 0 ;

		vector<Article>::iterator iteradorArtigo ;

		for (iteradorArtigo = artigos.begin() ; iteradorArtigo != artigos.end(); iteradorArtigo++)
		{
			sem_wait(sem_conexoes) ;

			sem_wait(&sem_artigosDown) ;

			pthread_mutex_lock(&mut_print) ;
			cout << "Criando thread processarArtigo " << iteradorArtigo->url << endl ;
			pthread_mutex_unlock(&mut_print) ;

			pthread_create(threads + i, NULL, processarArtigo, (void*) &*iteradorArtigo) ;
			i++ ;
		}

		for (i = 0 ; i < artigos.size(); i++)
		{
			pthread_join(threads[i], NULL) ;
		}

		delete(threads) ;

		pthread_mutex_lock(&mut_print) ;
		cout << "Thread processarFeed " << strUrl << " chegando ao fim, desalocando recursos." << endl ;
		pthread_mutex_unlock(&mut_print) ;

		std::string str = strServidor ;

		servidorConexoes.erase(strServidor) ;

		delete (sem_conexoes) ;
	}

	finalizacao:

	sem_post(&sem_feedsDown) ;
}


static void *processarArtigo(void *art)
{
	Article &artigo = *(Article*) art ;

	HTMLDocument documento(artigo.url) ;

	string strServidor = getURLServer(artigo.url) ;

	try
	{
		documento.parse() ;
	}
	catch (HTMLDocumentException &htmlEx)
	{
		/*
		 * Pagina as vezes possui elementos que a biblioteca nao consegue processar, e retorna erro.
		 */

		pthread_mutex_lock(&mut_print) ;
		cerr << "Problema ao processar artigo " << artigo.url << " , abortando processamento e finalizando thread." << endl ;
		pthread_mutex_unlock(&mut_print) ;

		goto finalizacao ;
	}

	pthread_mutex_lock(&mut_index) ;
	index.add(artigo, documento.getTokens()) ;
	pthread_mutex_unlock(&mut_index) ;

	pthread_mutex_lock(&mut_print) ;
	cout << "Thread processarArtigo " << artigo.url << " concluida." << endl ;
	pthread_mutex_unlock(&mut_print) ;

	finalizacao:

	if (servidorConexoes.find(strServidor)->second != 0)
	{
		/*
		 * As vezes o link original do artigo redireciona para outra URL, que pode ser de outro servidor com
		 * um prefixo diferente. Quando isso acontece o prefixo atual da URL não terá uma entrada na hash table, e o
		 * programa levará segfault caso essa checagem não aconteça.
		 */
		sem_post(servidorConexoes.find(strServidor)->second) ;
	}

	sem_post(&sem_artigosDown) ;
}


static void sequencial(const string& feedListURI)
{
	RSSFeedList feedList(feedListURI) ;
	map<string, string>::iterator it ;
	auto feeds = feedList.getFeeds() ;

	for (it = feeds.begin() ; it != feeds.end(); it++)
	{
		RSSFeed feed(it->first) ;
		feed.parse() ;

		vector<Article>::iterator itArtigo ;
		auto artigos = feed.getArticles() ;

		for (itArtigo = artigos.begin() ; itArtigo != artigos.end(); itArtigo++)
		{
			HTMLDocument documento(itArtigo->url) ;
			documento.parse() ;

			Article &artigoAtual = *itArtigo ;

			index.add(artigoAtual, documento.getTokens()) ;

			vector<string>::iterator itTokens ;
			auto tokens = documento.getTokens() ;
		}
	}
}


static const size_t kMaxMatchesToShow = 15;
static void queryIndex() {
  while (true) {
    cout << "Enter a search term [or just hit <enter> to quit]: ";
    string response;
    getline(cin, response);
    response = trim(response);
    if (response.empty()) break;
    const vector<pair<Article, int> >& matches = index.getMatchingArticles(response);
    if (matches.empty()) {
      cout << "Ah, we didn't find the term \"" << response << "\". Try again." << endl;
    } else {
      cout << "That term appears in " << matches.size() << " article" 
	   << (matches.size() == 1 ? "" : "s") << ".  ";
      if (matches.size() > kMaxMatchesToShow) 
	cout << "Here are the top " << kMaxMatchesToShow << " of them:" << endl;
      else if (matches.size() > 1)
	cout << "Here they are:" << endl;
      else
        cout << "Here it is:" << endl;
      size_t count = 0;
      for (const pair<Article, int>& match: matches) {
	if (count == kMaxMatchesToShow) break;
	count++;
	string title = match.first.title;
	if (shouldTruncate(title)) title = truncate(title);
	string url = match.first.url;
	if (shouldTruncate(url)) url = truncate(url);
	string times = match.second == 1 ? "time" : "times";
	cout << "  " << setw(2) << setfill(' ') << count << ".) "
	     << "\"" << title << "\" [appears " << match.second << " " << times << "]." << endl;
	cout << "       \"" << url << "\"" << endl;
      }
    }
  }
}

int main(int argc, char *argv[]) {
  string rssFeedListURI = parseArgumentList(argc, argv);
  xmlInitParser();
  xmlInitializeCatalog();
  processAllFeeds(rssFeedListURI);
  xmlCatalogCleanup();
  xmlCleanupParser();
  queryIndex();
  return 0;
}
