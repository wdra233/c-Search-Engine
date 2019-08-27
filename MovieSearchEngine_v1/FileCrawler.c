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
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#include "FileCrawler.h"
#include "DocIdMap.h"
#include "LinkedList.h"


void CrawlFilesToMap(const char *dir, DocIdMap map) {
  struct stat s;

  struct dirent **namelist;
  int n;
  n = scandir(dir, &namelist, 0, alphasort);

  printf("crawling dir: %s\n", dir);
  if (n < 0) {
    perror("scandir");
    printf("dir: %s\n", dir);
    return;
  } else {
    int i = 0;
    while (i < n) {
      if (namelist[i]->d_name[0] == '.') {
        free(namelist[i]);
        i++;
        continue;
      }
      char *directory;
      directory = (char*) malloc(((strlen(dir) +
                                   (strlen(namelist[i]->d_name)) +
                                   2)*sizeof(char)));
      if (directory == NULL) {
        printf("Couldn't malloc for filecrawler.directory\n");
        return;
      }
      strcpy(directory, dir);
      strcat(directory, namelist[i]->d_name);

      if (0 == stat(directory, &s)) {
        if (S_ISDIR(s.st_mode)) {
          strcat(directory, "/");
          CrawlFilesToMap(directory, map);
          free(directory);
        } else {
          printf("adding file to map: %s\n", directory);
          PutFileInMap(directory, map);
        }
      } else {
        printf("no stat; %s\n", directory);
      }
      free(namelist[i]);
      i++;
    }
  }
  free(namelist);
}
