# define the commands we'll use for compilation and library building
AR = ar
ARFLAGS = rcs
CC = gcc

GOOGLE_TEST_INCLUDE=${HOME}/src/googletest-release-1.8.0/googletest/include/

# define useful flags to cc/ld/etc.
CFLAGS = -g -fPIC  -Wall -I. -I.. -Ihtll -pthread
LDFLAGS = -L.


#define common dependencies
OBJS = MovieSet.o DocIdMap.o FileParser.o FileCrawler.o MovieIndex.o Assert007.o Movie.o QueryProcessor.o MovieReport.o
HEADERS = FileParser.h FileCrawler.h DocIdMap.h MovieIndex.h MovieSet.h Movie.h Assert007.h MovieReport.h


# compile everything
all: main indexer benchmarker


main: main.c $(OBJS)
	gcc -Wall -g -o main main.c $(OBJS) -L. libHtll.a
	@echo ===========================
	@echo Run main by running ./main dir/
	@echo \(where dir is the directory to look for files to index\)
	@echo ===========================

indexer: example_indexer.c $(OBJS)
	gcc -Wall -g -o indexer example_indexer.c $(OBJS) -L. libHtll.a
	@echo ===========================
	@echo Run indexer by running ./indexer -X filename
	@echo \(X is the field to index by, filename is the file of movies to index\)
	@echo ===========================

benchmarker: Benchmarker.c $(OBJS)
	gcc -Wall -g -o benchmarker Benchmarker.c $(OBJS) -L. libHtll.a -pthread
	@echo ===========================
	@echo Run benchmarker by running ./benchmarker dir_name/
	@echo \(dirname  is the file of movies to use for benchmark\)
	@echo ===========================

%.o: %.c $(HEADERS) FORCE
	$(CC) $(CFLAGS) -c $<

clean: FORCE
	/bin/rm -f *.o *~ main indexer benchmarker

FORCE:
