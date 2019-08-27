#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>


#include "includes/QueryProtocol.h"
#include "MovieSet.h"
#include "MovieIndex.h"
#include "DocIdMap.h"
#include "htll/Hashtable.h"
#include "QueryProcessor.h"
#include "FileParser.h"
#include "FileCrawler.h"

#define BUFFER_SIZE 1000

int Cleanup();

DocIdMap docs;
Index docIndex;

#define SEARCH_RESULT_LENGTH 1500
#define RESPONSE 50
#define DIR_BUFFER 100
#define PORT_BUFFER 100
#define NUM_ARGS 3
char movieSearchResult[SEARCH_RESULT_LENGTH];

// Edited by Yawang Wang since Apr 9.
// Completed on Apr 21.

void sigchld_handler(int s) {
    write(0, "Handling zombies...\n", 20);
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


void sigint_handler(int sig) {
    write(0, "Ahhh! SIGINT!\n", 14);
    Cleanup();
    exit(0);
}

/**
 *
 */
int HandleConnections(int sock_fd) {
    // Step 5: Accept connection
    // Fork on every connection.
    pid_t pid;
    int client_fd;
    char number[RESPONSE];
    while (1) {
        printf("waiting for connection...\n");
        client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("Connection()");
            exit(-1);
        }
        printf("Connection made: client_fd=%d\n", client_fd);
        pid = fork();
        if (pid == -1) perror("Run out of memory!");
        if (pid == 0) {
            while (1) {
                printf("handling client\n");
                if (SendAck(client_fd) < 0) {
                    perror("SendAck()");
                    exit(-1);
                }
                char buffer[BUFFER_SIZE];
                int len = read(client_fd, buffer, sizeof(buffer) - 1);
                buffer[len] = '\0';
                char *term = buffer;
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
                        exit(-1);
                    }

                    num_response = results->numResults;

                    printf("Num of responses: %d\n", num_response);

                    snprintf(number, sizeof(number), "%d", num_response);
                    write(client_fd, number, strlen(number));
                    len = read(client_fd, buffer, sizeof(buffer) - 1);
                    buffer[len] = '\0';
                    if (CheckAck(buffer) == -1) {
                        perror("No ACK from Client!");
                        exit(-1);
                    }

                    while (num_response > 0) {
                        SearchResultGet(results, sr);
                        CopyRowFromFile(sr, docs, movieSearchResult);
                        write(client_fd, movieSearchResult,
                        strlen(movieSearchResult));
                        len = read(client_fd, buffer, sizeof(buffer) - 1);
                        buffer[len] = '\0';
                        if (CheckAck(buffer) == -1) {
                            perror("No ACK from Client!");
                            exit(-1);
                        }
                        SearchResultNext(results);
                        num_response--;
                    }
                    free(sr);
                    printf("Destroying SearchResult Iterator...\n");
                    DestroySearchResultIter(results);
                }
                if (SendGoodbye(client_fd) == -1) {
                    perror("Goodbye fail to send!");
                    exit(-1);
                }
                exit(0);
            }
        } else {
            signal(SIGCHLD, sigchld_handler);
            close(client_fd);
        }
    }
    return 0;
}

void Setup(char *dir) {
    struct sigaction sa;

    sa.sa_handler = sigchld_handler;  // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    struct sigaction kill;

    kill.sa_handler = sigint_handler;
    kill.sa_flags = 0;  // or SA_RESTART
    sigemptyset(&kill.sa_mask);

    if (sigaction(SIGINT, &kill, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

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

int main(int argc, char **argv) {
    // Get args
    if (argc != NUM_ARGS) {
        printf("Incorrect number of arguments! "
        "The number of arguments should be 3.\n");
        printf("Multiserver is started by calling: "
        "./multiserver [datadir] [port]\n");
        exit(1);
    }
    char dir_to_crawl[DIR_BUFFER];
    // handle input directory properly.
    snprintf(dir_to_crawl, sizeof(dir_to_crawl), "%s%s", argv[1], "/");
    char portBuffer[PORT_BUFFER];
    snprintf(portBuffer, sizeof(portBuffer), "%s", argv[2]);
    Setup(dir_to_crawl);

    // Step 1: Get address stuff
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
    // Step 5: Handle the connections
    freeaddrinfo(result);
    HandleConnections(sock_fd);

    // Got Kill signal
    close(sock_fd);
    Cleanup();
    return 0;
}
