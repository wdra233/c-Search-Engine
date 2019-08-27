#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#include "includes/QueryProtocol.h"
#include "MovieSet.h"
#include "MovieIndex.h"
#include "DocIdMap.h"
#include "htll/Hashtable.h"
#include "includes/QueryProcessor.h"
#include "FileParser.h"
#include "FileCrawler.h"

#define BUFFER_SIZE 1000
#define SEARCH_RESULT_LENGTH 1500
#define PORT_BUFFER 100
#define DIR_BUFFER 100
#define RESPONSE 30
#define NUM_ARGS 3
DocIdMap docs;
Index docIndex;
char movieSearchResult[SEARCH_RESULT_LENGTH];
char portBuffer[PORT_BUFFER];
// Edited by Yawang Wang since Apr 9.
// Completed on Apr 21.

int Cleanup();

void sigint_handler(int sig) {
    write(0, "Exit signal sent. Cleaning up...\n", 34);
    Cleanup();
    exit(0);
}


void Setup(char *dir) {
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

int Cleanup() {
    DestroyOffsetIndex(docIndex);
    DestroyDocIdMap(docs);

    return 0;
}

int HandleSingleConnection(int sock_fd) {
    while (1) {
        printf("Waiting for connection...\n");
        int client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("Connection()");
            exit(1);
        }
        printf("Connection made!\n");
        if (SendAck(client_fd) < 0) {
            perror("SendAck()");
            exit(1);
        }
        char buffer[BUFFER_SIZE];
        int len = read(client_fd, buffer, sizeof(buffer) - 1);
        buffer[len] = '\0';
        char *term = buffer;
        char number[RESPONSE];
        SearchResultIter results = FindMovies(docIndex, term);
        int num_response = 0;
        if (results == NULL) {
            printf("No results for this term. Please try another.\n");
            snprintf(number, sizeof(number), "%d", num_response);
            write(client_fd, number, strlen(number));
        } else {
            SearchResult sr = (SearchResult) malloc(sizeof(*sr));
            if (sr == NULL) {
                printf("Couldn't malloc SearchResult in main.c\n");
                exit(1);
            }

            num_response = results->numResults;

            printf("Num of responses: %d\n", num_response);

            snprintf(number, sizeof(number), "%d", num_response);
            write(client_fd, number, strlen(number));
            len = read(client_fd, buffer, sizeof(buffer) - 1);
            buffer[len] = '\0';
            if (CheckAck(buffer) == -1) {
                perror("No ACK from Client!");
                exit(1);
            }

            while (num_response > 0) {
                SearchResultGet(results, sr);
                CopyRowFromFile(sr, docs, movieSearchResult);
                write(client_fd, movieSearchResult, strlen(movieSearchResult));
                len = read(client_fd, buffer, sizeof(buffer) - 1);
                buffer[len] = '\0';
                if (CheckAck(buffer) == -1) {
                    perror("No ACK from Client!");
                    exit(1);
                }
                SearchResultNext(results);
                num_response--;
            }
            free(sr);
            printf("Destroying SearchResult Iterator...\n");
            DestroySearchResultIter(results);
        }
        // Step 6: Close the socket
        if (SendGoodbye(client_fd) == -1) {
            perror("Goodbye fail to send!");
            exit(1);
        }
        printf("Closing connection...\n");
        close(client_fd);
    }
    return 0;
}

int main(int argc, char **argv) {
    // Get args
    if (argc != NUM_ARGS) {
        printf("Incorrect number of arguments! "
        "The number of arguments should be 3.\n");
        printf("Client is started by calling: "
        "./queryserver [datadir] [port]\n");
        exit(1);
    }
    char dir_to_crawl[DIR_BUFFER];
    // handle input directory properly
    snprintf(dir_to_crawl, sizeof(dir_to_crawl), "%s%s", argv[1], "/");
    snprintf(portBuffer, sizeof(portBuffer), "%s", argv[2]);
    // Setup graceful exit
    struct sigaction kill;

    kill.sa_handler = sigint_handler;
    kill.sa_flags = 0;  // or SA_RESTART
    sigemptyset(&kill.sa_mask);

    if (sigaction(SIGINT, &kill, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    Setup(dir_to_crawl);
    // Step 1: get address/port info to open
    int s;
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    s = getaddrinfo(NULL, portBuffer, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    }
    // Step 2: Open socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    // Step 3: Bind socket
    if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0) {
        printf("Address already in use, please reuse it later.\n");
        exit(1);
    }
    // Step 4: Listen on the socket
    if (listen(sock_fd, 10) != 0) {
        perror("listen()");
        exit(1);
    }
    // Step 5: Handle clients that connect
    freeaddrinfo(result);
    HandleSingleConnection(sock_fd);
    // Got Kill signal
    close(sock_fd);

    Cleanup();

    return 0;
}
