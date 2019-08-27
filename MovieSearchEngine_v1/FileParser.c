/*
 *  Created by Adrienne Slaughter
 *  CS 5007 Spring 2019
 *  Northeastern University, Seattle
 *
 *  This is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  It is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  See <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include "MovieIndex.h"
#include "FileParser.h"
#include "Movie.h"
#include "DocIdMap.h"
#include "MovieSet.h"
// Edited by Yawang Wang since Apr 4.
// Finished on Apr 8.
//  Only for NullFree; TODO(adrienne): NullFree should live somewhere else.

#define BUFFER_SIZE 1000
#define NUM_THREADS 5

typedef struct property {
    HTIter iter;
    Index index;
} proper, *Property;
pthread_t thread[NUM_THREADS];
int ptrThread = -1;
pthread_mutex_t mutex, tmutex;

//=======================
// To minimize the number of files we have, I'm
// putting the private function prototypes for
// the fileparser here.

void IndexTheFile(char *file, uint64_t docId, Index index);

// void IndexTheFile_MT(char *file, uint64_t docId, Index index);


/**
 * \fn Parses the files that are in a provided DocIdMap.
 * Builds an OffsetIndex
 */
int ParseTheFiles(DocIdMap docs, Index index) {
    HTIter iter = CreateHashtableIterator(docs);
    HTKeyValue kv;

    HTIteratorGet(iter, &kv);
    IndexTheFile(kv.value, kv.key, index);

    while (HTIteratorHasMore(iter) != 0) {
        HTIteratorGet(iter, &kv);
        IndexTheFile(kv.value, kv.key, index);
        HTIteratorNext(iter);
    }

    HTIteratorGet(iter, &kv);
    IndexTheFile(kv.value, kv.key, index);

    DestroyHashtableIterator(iter);

    return 0;
}

// Builds an OffsetIndex
void IndexTheFile(char *file, uint64_t doc_id, Index index) {
    FILE *cfPtr;

    if ((cfPtr = fopen(file, "r")) == NULL) {
        printf("File could not be opened\n");
        return;
    } else {
        char buffer[BUFFER_SIZE];
        int row = 0;
        while (fgets(buffer, BUFFER_SIZE, cfPtr) != NULL) {
            Movie *movie = CreateMovieFromRow(buffer);
            if (movie == NULL) {
                continue;
            }
            int result = AddMovieTitleToIndex(index, movie, doc_id, row);
            if (result < 0) {
                fprintf(stderr, "Didn't add MovieToIndex.\n");
            }
            row++;
            DestroyMovie(movie);  // Done with this now
        }
        fclose(cfPtr);
    }
}

// Takes a linkedlist of movies, and builds a hashtable based on the given field
// Builds a TypeIndex
Index BuildMovieIndex(LinkedList movies, enum IndexField field_to_index) {
    Index movie_index = CreateIndex();
    movie_index->movies = movies;

    LLIter iter = CreateLLIter(movies);
    Movie *cur_movie;
    LLIterGetPayload(iter, (void **) &cur_movie);

    AddMovieToIndex(movie_index, cur_movie, field_to_index);

    while (LLIterHasNext(iter)) {
        LLIterNext(iter);
        LLIterGetPayload(iter, (void **) &cur_movie);
        AddMovieToIndex(movie_index, cur_movie, field_to_index);
    }

    DestroyLLIter(iter);

    return movie_index;
}


// Takes a linkedlist of movies, and builds a hashtable based on the given field
// Adds a movie to a TypeIndex
Index AddToMovieIndex(Index movie_index,
                      LinkedList movies,
                      enum IndexField field_to_index) {
    LLIter iter = CreateLLIter(movies);
    Movie *cur_movie;
    LLIterGetPayload(iter, (void **) &cur_movie);

    AddMovieToIndex(movie_index, cur_movie, field_to_index);
    InsertLinkedList(movie_index->movies, cur_movie);

    while (LLIterHasNext(iter)) {
        LLIterNext(iter);
        LLIterGetPayload(iter, (void **) &cur_movie);
        AddMovieToIndex(movie_index, cur_movie, field_to_index);
        InsertLinkedList(movie_index->movies, cur_movie);
    }

    DestroyLLIter(iter);

    //  NullFree because the payloads now live in the HT
    DestroyLinkedList(movies, NullFree);

    return movie_index;
}

// Returns a LinkedList of Movie structs from the specified file
LinkedList ReadFile(const char *filename) {
    FILE *cfPtr;

    LinkedList movie_list = CreateLinkedList();

    if ((cfPtr = fopen(filename, "r")) == NULL) {
        printf("File could not be opened\n");
        DestroyLinkedList(movie_list, NULL);
        return NULL;
    } else {
        char row[BUFFER_SIZE];

        int i = 0;

        while (!feof(cfPtr)) {
            fgets(row, BUFFER_SIZE, cfPtr);
            i++;
            // Got the line; create a movie from it
            MoviePtr movie = CreateMovieFromRow(row);
            if (movie != NULL) {
                InsertLinkedList(movie_list, movie);
            }
        }
        fclose(cfPtr);
    }
    return movie_list;
}

// Using pthread to run IndexTheFile.
void *IndexTheFile_MT(void *args) {
    Property property = (Property) args;
    HTKeyValue kv;
    while (1) {
        pthread_mutex_lock(&tmutex);
        if (HTIteratorHasMore(property->iter)) {
            HTIteratorGet(property->iter, &kv);
            HTIteratorNext(property->iter);
            pthread_mutex_unlock(&tmutex);
        } else {
            pthread_mutex_unlock(&tmutex);
            break;
        }
        pthread_mutex_lock(&mutex);
        IndexTheFile(kv.value, kv.key, property->index);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

// Call 5 threads in this function to run IndexTheFile_MT.
// Then, join the 5 threads.
int ParseTheFiles_MT(DocIdMap docs, Index index) {
    Property property = malloc(sizeof(proper));
    property->iter = CreateHashtableIterator(docs);
    property->index = index;
    HTKeyValue kv;
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&tmutex, NULL);
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&thread[i], NULL, IndexTheFile_MT, property);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(thread[i], NULL);
        printf("thread_%d done!\n", i + 1);
    }
    HTIteratorGet(property->iter, &kv);
    IndexTheFile(kv.value, kv.key, property->index);
    DestroyHashtableIterator(property->iter);
    free(property);
    return 0;
}
