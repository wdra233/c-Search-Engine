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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "htll/LinkedList.h"
#include "MovieIndex.h"
#include "Movie.h"
#include "MovieReport.h"
#include "FileParser.h"


int main(int argc, char* argv[]) {
  int opt;
  enum IndexField ind_field = Genre;
  char *filename;
  while ((opt = getopt(argc, argv, "g:t:y:")) != -1) {
    switch (opt) {
      case 't':
      case 'T':
        printf("sorting by type: %c\n", opt);
        ind_field = Type;
        filename = optarg;
        break;
      case 'g':
      case 'G':
        printf("sorting by genre: %s\n", optarg);
        ind_field = Genre;
        filename = optarg;
        break;
      case 'y':
      case 'Y':
        printf("sorting by year\n");
        ind_field = Year;
        filename = optarg;
        break;
      case ':':
        printf("option needs a value\n");
        break;
      case '?':
          printf("unknown option: %c\n", optopt);
      break;
    }
  }

  LinkedList movie_list  = ReadFile(filename);

  Index index = BuildMovieIndex(movie_list, ind_field);

  PrintReport(index);

  DestroyTypeIndex(index);

  return 0;
}
