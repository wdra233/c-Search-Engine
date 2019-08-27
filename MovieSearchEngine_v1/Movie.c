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
#include <string.h>
#include <ctype.h>


#include "Movie.h"

Movie* CreateMovie() {
  Movie *mov = (Movie*)malloc(sizeof(Movie));
  if (mov == NULL) {
    printf("Couldn't allocate more memory to create a Movie\n");
    return NULL;
  }
  mov->id = NULL;
  mov->type = NULL;
  mov->title = NULL;
  for (int i = 0; i < NUM_GENRES; i++) {
    mov->genres[i] = NULL;
  }
  mov->isAdult = -1;
  mov->runtime = -1;
  mov->year = -1;
  return mov;
}

void DestroyMovie(Movie* movie) {
  if (movie->id != NULL) free(movie->id);
  if (movie->type != NULL) free(movie->type);
  if (movie->title != NULL) free(movie->title);
  if (movie->genres != NULL) {
    for (int i = 0; i < NUM_GENRES; i++) {
      if (movie->genres[i] != NULL) free(movie->genres[i]);
    }
  }
  free(movie);
}


void DestroyMovieWrapper(void *movie) {
  DestroyMovie((Movie*)movie);
}


char* CheckAndAllocateString(char* token) {
  if (strcmp("-", token) == 0) {
    return NULL;
  } else {
    // TODO(adrienne): get rid of whitespace.
    char *out = (char *) malloc((strlen(token) + 1) * sizeof(char));
    strcpy(out, token);
    return out;
  }
}

int CheckInt(char* token) {
  if (strcmp("-", token) == 0) {
    return -1;
  } else {
    return atoi(token);
  }
}

Movie* CreateMovieFromRow(char *data_row) {
  int num_fields = 9;

  char *token[num_fields];
  char *rest = data_row;

  for (int i = 0; i < num_fields; i++) {
    token[i] = strtok_r(rest, "|", &rest);
    if (token[i] == NULL) {
      return NULL;
    }
  }
  Movie* mov = CreateMovie();
  if (mov == NULL) {
    printf("Couldn't create a Movie.\n");
    return NULL;
  }
  mov->id = CheckAndAllocateString(token[0]);
  mov->type = CheckAndAllocateString(token[1]);
  mov->title = CheckAndAllocateString(token[2]);
  mov->isAdult = CheckInt(token[4]);
  mov->year = CheckInt(token[5]);
  mov->runtime = CheckInt(token[7]);

  char *g_token;

  if (token[8][0] != '-') {
    for (int i = 0; i < NUM_GENRES; i++) {
      g_token = strtok_r(token[8], ",", &token[8]);
      if (g_token != NULL) {
        mov->genres[i] = CheckAndAllocateString(g_token);
      } else {
        break;
      }
    }
  }

  return mov;
}

void Trim(char* string) {
  int i = 0;
  int front_spaces = 0;

  while (isspace(string[i])) {
    i++;
    front_spaces++;
  }

  int cur_spot = 0;
  while ((string[i] != '\0')) {
    string[cur_spot++] = string[i++];
  }

  if (isspace(string[i-1])) {
    i--;
    while (isspace(string[i])) {
      i--;
    }
    string[i+1] = '\0';
  } else {
    string[i - front_spaces] = '\0';
  }
}
