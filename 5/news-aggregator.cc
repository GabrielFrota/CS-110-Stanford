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
#include <functional>

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
#include "thread-pool.h"
#include <unistd.h>
using namespace std;

// globals

static const size_t kFeedsPoolSize = 6;
static const size_t kArticlesPoolSize = 12;

static bool verbose = false;
static RSSIndex index;

static ThreadPool feedsPool(kFeedsPoolSize);
static ThreadPool articlesPool(kArticlesPoolSize);

static pthread_mutex_t mutexPrint, mutexIndex ;


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

static void *processarFeed(void *param);


static void *processarArtigo(void *art);


static const int kBogusRSSFeedListName = 1;
static void processAllFeeds(const string& feedListURI) {
  RSSFeedList feedList(feedListURI);
  try {
    feedList.parse();
  } catch (const RSSFeedListException& rfle) {
    cerr << "Ran into trouble while pulling full RSS feed list from \""
	 << feedListURI << "\"." << endl; 
    cerr << "Aborting...." << endl;
    exit(kBogusRSSFeedListName);
  }

  // note to student
  // ---------------
  // add well-decomposed code to read all of the RSS
  // news feeds from feedList for their news articles,
  // and for each news article URL, process it
  // as an HTMLDocument and add all of the tokens 
  // to the master RSSIndex.


  pthread_mutex_init(&mutexPrint, NULL);
  pthread_mutex_init(&mutexIndex, NULL);

  map<string, string>::iterator iteradorFeed ;
  auto feeds = feedList.getFeeds();

  for ( iteradorFeed=feeds.begin(); iteradorFeed!=feeds.end(); iteradorFeed++ ) {

  	pthread_mutex_lock(&mutexPrint);
  	cout << "Enfileirando chamada processarFeed " << iteradorFeed->first << endl ;
  	pthread_mutex_unlock(&mutexPrint);

  	/*
  	 * esta funçao processAllFeeds nao vai retornar antes que todos os feeds enfileirados por ela sejam processados pelo thread pool,
  	 * portanto o endereço passado a funçao processarFeed pode ser uma variavel local.
  	 */

  	Funcao temp ;
  	temp.f = processarFeed ;
  	temp.param = (void*)&(iteradorFeed->first) ;

  	feedsPool.schedule( temp ) ;

  }

  feedsPool.wait() ;

  articlesPool.wait() ;

  pthread_mutex_destroy(&mutexPrint);
  pthread_mutex_destroy(&mutexIndex);

}


static void *processarFeed(void *param) {

	string &strUrl = *(string*)param ;

	RSSFeed feed( strUrl );

	try {

		feed.parse();

	} catch (RSSFeedException &rssEx) {

		/*
		 * URL informada no arquivo xml pode ser invalida, ou a pagina possui elementos que a biblioteca nao consegue processar.
		 */

		pthread_mutex_lock(&mutexPrint);
		cerr << "Problema ao processar feed " << strUrl << " , abortando processamento e retornando." << endl ;
		pthread_mutex_unlock(&mutexPrint);

		return 0 ;

	}

	{

	auto artigos = feed.getArticles();

	if (artigos.size() == 0) {

		/*
		 *  A biblioteca usada é uma biblioteca C, e a camada que joga exceçoes é a camada C++ fornecida no trabalho.
		 *  As vezes nao acontece exceçao na camada C++, mas a biblioteca retorna uma lista de artigos vazia pois algum problema
		 *  aconteceu.
		 */

		pthread_mutex_lock(&mutexPrint);
		cerr << "Lista de artigos do feed " << strUrl << " invalida, abortando processamento e retornando." << endl ;
		pthread_mutex_unlock(&mutexPrint);

		return 0 ;

	}

	/*
	 *  Obtendo a url do servidor pela url dos artigos, pois as vezes a url fornecido no arquivo xml gera um redirect para outra
	 *  url com um prefixo diferente, e isso causa problemas pois o prefixo do servidor da url do xml e da url dos artigos vai
	 *  ser diferente.
	 */

	string strServidor = getURLServer( artigos.begin()->url );

	vector<Article>::iterator iteradorArtigo ;

	for ( iteradorArtigo=artigos.begin(); iteradorArtigo!=artigos.end() ; iteradorArtigo++ ) {

		pthread_mutex_lock(&mutexPrint);
		cout << "Enfileirando chamada processarArtigo " << iteradorArtigo->url << endl ;
		pthread_mutex_unlock(&mutexPrint);

		/*
		 * esta chamada processarFeed vai acabar antes que todos os artigos enfileirados por ela sejam processados pelo outro thread pool,
		 * portanto o endereço passado a chamada processarArtigo nao pode ser o valor do iteradorArtigo, que tem um endereço do vetor local artigos.
		 *
		 * É preciso alocar structs Article no heap, copiar os valores, e cada chamada processarArtigo da free no seu Article quando terminar
		 * o processamento.
		 */

		Article *art = new Article ;
		art->title = iteradorArtigo->title ;
		art->url = iteradorArtigo->url ;

		Funcao temp ;
		temp.f = processarArtigo ;
		temp.param = (void*)art ;

		articlesPool.schedule( temp ) ;

	}

	pthread_mutex_lock(&mutexPrint);
	cout << "Chamada processarFeed " << strUrl << " chegando ao fim." << endl ;
	pthread_mutex_unlock(&mutexPrint);

	}

}


static void *processarArtigo(void *art) {

	Article &artigo = *(Article*)art ;

	HTMLDocument documento( artigo.url );

	string strServidor = getURLServer( artigo.url );

	try {

		documento.parse();

	} catch (HTMLDocumentException &htmlEx) {

		/*
		 * Pagina as vezes possui elementos que a biblioteca nao consegue processar, e retorna erro.
		 */

		pthread_mutex_lock(&mutexPrint);
		cerr << "Problema ao processar artigo " << artigo.url << " , abortando processamento e retornando." << endl ;
		pthread_mutex_unlock(&mutexPrint);

		return 0 ;

	}

	pthread_mutex_lock(&mutexIndex);
	index.add(artigo, documento.getTokens() );
	pthread_mutex_unlock(&mutexIndex);

	pthread_mutex_lock(&mutexPrint);
	cout << "Chamada processarArtigo " << artigo.url << " chegando ao fim. Desalocando struct Article." << endl ;
	pthread_mutex_unlock(&mutexPrint);

	free(art) ;

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
