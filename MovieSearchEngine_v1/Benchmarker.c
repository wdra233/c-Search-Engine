#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>

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
#define BUFFER_SIZE 1000
// Edited by Yawang Wang since Apr 4.
// Finished on Apr 8.
DocIdMap docs;
Index docIndex;
Index movie_index;
char *searchGenre = "Comedy";
char *searchWord = "Seattle";

/**
 * Open the specified file, read the specified row into the
 * provided pointer to the movie.
 */
int CreateMovieFromFileRow(char *file, int rowId, Movie **movie) {
    FILE *fp;

    char buffer[1000];

    fp = fopen(file, "r");

    int i = 0;
    while (i <= rowId) {
        fgets(buffer, 1000, fp);
        i++;
    }
    // taking \n out of the row
    buffer[strlen(buffer) - 1] = ' ';
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

// Use an OffsetIndex, get a list of movies that has Seattle in the title,
// Search through that list and find the ones that are Comedy.
void runQueryOnOffsetIndex(char *term) {
    // Figure out how to get a set of Movies and create
    // a nice report from them.
    // Then print them out.
    SearchResultIter results = FindMovies(docIndex, term);
    LinkedList movies = CreateLinkedList();

    if (results == NULL) {
        printf("No results for this term. Please try another.\n");
        return;
    } else {
        SearchResult sr = (SearchResult) malloc(sizeof(*sr));
        if (sr == NULL) {
            printf("Couldn't malloc SearchResult in Benchmarker.c\n");
            return;
        }
        int result;
        char *filename;

        // Get the last
        SearchResultGet(results, sr);
        filename = GetFileFromId(docs, sr->doc_id);

        Movie *movie;
        CreateMovieFromFileRow(filename, sr->row_id, &movie);
        for (int i = 0; i < NUM_GENRES; i++) {
            if (movie->genres[i] != NULL &&
            strstr(movie->genres[i], searchGenre) != NULL) {
                InsertLinkedList(movies, movie);
                break;
            }
        }

        // Check if there are more
        while (SearchResultIterHasMore(results) != 0) {
            result = SearchResultNext(results);
            if (result < 0) {
                printf("error retrieving result\n");
                break;
            }
            SearchResultGet(results, sr);
            char *filename = GetFileFromId(docs, sr->doc_id);

            Movie *movie;
            CreateMovieFromFileRow(filename, sr->row_id, &movie);
            for (int i = 0; i < NUM_GENRES; i++) {
                if (movie->genres[i] != NULL &&
                strstr(movie->genres[i], searchGenre) != NULL) {
                    InsertLinkedList(movies, movie);
                    break;
                }
            }
        }

        free(sr);
        DestroySearchResultIter(results);
    }
    if (NumElementsInLinkedList(movies) == 0) {
        printf("0 items found in %s genre!\n", searchGenre);
        return;
    }
    OutputListOfMovies(movies, searchGenre, stdout);
    DestroyLinkedList(movies, DestroyMovieWrapper);
}

// Use a TypeIndex, get a list of movies that are Comedy,
// find the ones that have Seattle in the title.
// Then print them out.
void runQueryOnTypeIndex(Index movie_index) {
    uint64_t genre_key = FNVHash64((unsigned char *) searchGenre,
    strlen(searchGenre));
    HTKeyValue kv;
    // char temp_title[1000];
    int result = LookupInHashtable(movie_index->ht, genre_key, &kv);
    if (result < 0) {
        printf("No results!\n");
        return;
    }
    printf("Search items that are %s genre...\n", searchGenre);
    int cnt = 0;
    LinkedList movie_list = ((SetOfMovies) kv.value)->movies;
    LLIter iter = CreateLLIter(movie_list);
    Movie *movie;

    while (LLIterHasNext(iter)) {
        LLIterGetPayload(iter, (void **) &movie);
        // Use strstr to find titles that contain seattle.
        if (strstr(movie->title, searchWord) != NULL) {
            cnt++;
            printf("\t%s\n", movie->title);
        }
        LLIterNext(iter);
    }
    LLIterGetPayload(iter, (void **) &movie);
    if (strstr(movie->title, searchWord) != NULL) {
        cnt++;
        printf("\t%s\n", movie->title);
    }
    printf("%d items found with %s title!\n", cnt, searchWord);
    DestroyLLIter(iter);
}

void BenchmarkSetOfMovies(DocIdMap docs) {
    HTIter iter = CreateHashtableIterator(docs);
    // Now go through all the files, and insert them into the index.
    HTKeyValue kv;
    HTIteratorGet(iter, &kv);
    LinkedList movie_list = ReadFile((char *) kv.value);
    movie_index = BuildMovieIndex(movie_list, Genre);

    while (HTIteratorHasMore(iter) != 0) {
        HTIteratorGet(iter, &kv);
        movie_list = ReadFile((char *) kv.value);
        AddToMovieIndex(movie_index, movie_list, Genre);
        HTIteratorNext(iter);
    }
    printf("%d entries in the index.\n", NumElemsInHashtable(movie_index->ht));
    DestroyHashtableIterator(iter);
}

void BenchmarkMovieSet(DocIdMap docs) {
    // Create the index
    docIndex = CreateIndex();

    // Index the files
    printf("Parsing and indexing files...\n");
    ParseTheFiles(docs, docIndex);
    printf("%d entries in the index.\n", NumElemsInHashtable(docIndex->ht));
}

void BenchmarkMultiThread(DocIdMap docs) {
    docIndex = CreateIndex();
    printf("MultiThread initializing....\n");
    ParseTheFiles_MT(docs, docIndex);
    printf("%d entries in the index.\n", NumElemsInHashtable(docIndex->ht));
}

void WriteFile(FILE *file) {
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, file) != NULL) {
        printf("%s\n", buffer);
    }
}

/*
 * Measures the current (and peak) resident and virtual memories
 * usage of your linux C process, in kB
 */
void getMemory() {
    //    int* currRealMem, int* peakRealMem,
    //    int* currVirtMem, int* peakVirtMem) {

    int currRealMem, peakRealMem, currVirtMem, peakVirtMem;

    // stores each word in status file
    char buffer[1024] = "";

    // linux file contains this-process info
    FILE *file = fopen("/proc/self/status", "r");

    // read the entire file
    while (fscanf(file, " %1023s", buffer) == 1) {
        if (strcmp(buffer, "VmRSS:") == 0) {
            fscanf(file, " %d", &currRealMem);
        }
        if (strcmp(buffer, "VmHWM:") == 0) {
            fscanf(file, " %d", &peakRealMem);
        }
        if (strcmp(buffer, "VmSize:") == 0) {
            fscanf(file, " %d", &currVirtMem);
        }
        if (strcmp(buffer, "VmPeak:") == 0) {
            fscanf(file, " %d", &peakVirtMem);
        }
    }

    fclose(file);

    printf("Cur Real Mem: %d\tPeak Real Mem: %d "
           "\t Cur VirtMem: %d\tPeakVirtMem: %d\n",
           currRealMem, peakRealMem,
           currVirtMem, peakVirtMem);
}

int main(int argc, char *argv[]) {
    // Check arguments
    if (argc != 2) {
        printf("Wrong number of arguments.\n");
        printf("usage: main <directory_to_crawl>\n");
        return 0;
    }
    pid_t pid = getpid();
    printf("Process ID: %d\n", pid);
    getMemory();

    // Create a DocIdMap
    docs = CreateDocIdMap();
    CrawlFilesToMap(argv[1], docs);
    printf("Crawled %d files.\n", NumElemsInHashtable(docs));
    printf("Created DocIdMap\n");

    getMemory();

    clock_t start2, end2;
    double cpu_time_used;



    // ======================
    // Benchmark SetOfMovies
    printf("\n\nBuilding the TypeIndex\n");
    start2 = clock();
    BenchmarkSetOfMovies(docs);
    end2 = clock();
    cpu_time_used = ((double) (end2 - start2)) / CLOCKS_PER_SEC;
    printf("Took %f seconds to execute. \n", cpu_time_used);
    printf("Memory usage: \n");
    getMemory();
    printf("\n\nRun query based on the TypeIndex\n");
    start2 = clock();
    runQueryOnTypeIndex(movie_index);
    end2 = clock();
    cpu_time_used = ((double) (end2 - start2)) / CLOCKS_PER_SEC;
    printf("Took %f seconds to execute. \n", cpu_time_used);
    printf("Memory usage: \n");
    getMemory();
    DestroyTypeIndex(movie_index);
    printf("Destroyed TypeIndex\n");
    getMemory();

    // ======================
    printf("\n\nBuilding the OffsetIndex\n");
    start2 = clock();
    BenchmarkMovieSet(docs);
    end2 = clock();
    cpu_time_used = ((double) (end2 - start2)) / CLOCKS_PER_SEC;
    printf("Took %f seconds to execute. \n", cpu_time_used);
    printf("Memory usage: \n");
    getMemory();

    printf("\n\nRun query based on OffsetIndex\n");
    start2 = clock();
    runQueryOnOffsetIndex(searchWord);
    end2 = clock();
    cpu_time_used = ((double) (end2 - start2)) / CLOCKS_PER_SEC;
    printf("Took %f seconds to execute. \n", cpu_time_used);
    printf("Memory usage: \n");
    getMemory();
    DestroyOffsetIndex(docIndex);
    printf("Destroyed OffsetIndex\n");
    getMemory();
    //=======================
    printf("\n\nBuilding the MultiThread on OffsetIndex\n");
    start2 = clock();
    BenchmarkMultiThread(docs);
    end2 = clock();
    cpu_time_used = ((double) (end2 - start2)) / CLOCKS_PER_SEC;
    printf("Took %f seconds to execute. \n", cpu_time_used);
    printf("Memory usage: \n");
    getMemory();
    DestroyOffsetIndex(docIndex);
    printf("Destroyed OffsetIndex\n");
    getMemory();


    //=======================
    DestroyDocIdMap(docs);
    printf("\n\nDestroyed DocIdMap\n");
    getMemory();
}
