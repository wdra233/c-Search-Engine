#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#include "htll/Hashtable.h"
#include "htll/LinkedList.h"

#include "MovieSet.h"
#include "DocIdMap.h"
#include "FileParser.h"
#include "FileCrawler.h"
#include "MovieIndex.h"
#include "Movie.h"
#include "QueryProcessor.h"
#include "MovieReport.h"


DocIdMap docs;
Index docIndex;


/**
 * Open the specified file, read the specified row into the
 * provided pointer to the movie.
 */
int CreateMovieFromFileRow(char *file, long rowId, Movie** movie) {
  FILE *fp;
  char buffer[1000];

  fp = fopen(file, "r");

  int i = 0;
  while (i <= rowId) {
    fgets(buffer, 1000, fp);
    i++;
  }
  // taking \n out of the row
  buffer[strlen(buffer)-1] = ' ';
  // Create movie from row
  *movie = CreateMovieFromRow(buffer);
  fclose(fp);
  return 0;
}

void doPrep(char *dir) {
  printf("Crawling directory tree starting at: %s\n", dir);
  // Create a DocIdMap
  docs = CreateDocIdMap();
  CrawlFilesToMap(dir, docs);

  printf("Crawled %d files.\n", NumElemsInHashtable(docs));

  // Create the index
  docIndex = CreateIndex();

  // Index the files
  printf("Parsing and indexing files...\n");
  ParseTheFiles(docs, docIndex);
  printf("%d entries in the index.\n", NumElemsInHashtable(docIndex->ht));
}

void runQuery(char *term) {
  // Figure out how to get a set of Movies and create
  // a nice report from them.
  SearchResultIter results = FindMovies(docIndex, term);
  LinkedList movies = CreateLinkedList();

  if (results == NULL) {
    printf("No results for this term. Please try another.\n");
    return;
  } else {
    SearchResult sr = (SearchResult)malloc(sizeof(*sr));
    if (sr == NULL) {
      printf("Couldn't malloc SearchResult in main.c\n");
      return;
    }
    int result;
    char *filename;

    // Get the last
    SearchResultGet(results, sr);
    filename = GetFileFromId(docs, sr->doc_id);

    Movie *movie;
    CreateMovieFromFileRow(filename, sr->row_id, &movie);
    InsertLinkedList(movies, movie);

    // Check if there are more
    while (SearchResultIterHasMore(results) != 0) {
      result =  SearchResultNext(results);
      if (result < 0) {
        printf("error retrieving result\n");
        break;
      }
      SearchResultGet(results, sr);
      char *filename = GetFileFromId(docs, sr->doc_id);

      Movie *movie;
      CreateMovieFromFileRow(filename, sr->row_id, &movie);
      InsertLinkedList(movies, movie);
    }

    free(sr);
    DestroySearchResultIter(results);
  }
  // Now that you have all the search results, print them out nicely.
  Index index = BuildMovieIndex(movies, Type);
  PrintReport(index);

  DestroyTypeIndex(index);
}

void runQueries() {
  char input[1000];
  while (1) {
    printf("\nEnter a term to search for, or q to quit: ");
    scanf("%s", input);

    if (strlen(input) == 1 &&
        (input[0] == 'q')) {
          printf("Thanks for playing! \n");
          return;
      }

    printf("\n");
    runQuery(input);
  }
}

int main(int argc, char *argv[]) {
  // Check arguments
  if (argc != 2) {
    printf("Wrong number of arguments.\n");
    printf("usage: main <directory_to_crawl>\n");
    return 0;
  }

  doPrep(argv[1]);
  runQueries();

  DestroyOffsetIndex(docIndex);

  DestroyDocIdMap(docs);
}
