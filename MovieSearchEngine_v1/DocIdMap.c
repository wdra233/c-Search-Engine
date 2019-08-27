#include <stdlib.h>
#include <stdio.h>
#include "DocIdMap.h"
#include "htll/Hashtable.h"

void DestroyString(void *val) {
    free(val);
}

DocIdMap CreateDocIdMap() {
  DocIdMap docs = (DocIdMap)CreateHashtable(64);
  return docs;
}

void DestroyDocIdMap(DocIdMap map) {
  DestroyHashtable(map, &DestroyString);
}

void PutFileInMap(char *filename, DocIdMap map) {
  HTKeyValue kvp;
  HTKeyValue old_kvp;
  kvp.key = NumElemsInHashtable(map) + 1;
  kvp.value = filename;
  int result = PutInHashtable(map, kvp, &old_kvp);
  if (result == 2) {
    printf("there was a duplicate!!\n");
  } else if (result == 0) {
    //    printf("inserted! \n");
  }
}

DocIdIter CreateDocIdIterator(DocIdMap map) {
  HTIter iter = CreateHashtableIterator(map);
  return iter;
}

void DestroyDocIdIterator(DocIdIter iter) {
  DestroyHashtableIterator(iter);
}

char *GetFileFromId(DocIdMap docs, int docId) {
  HTKeyValue kvp;
  int result = LookupInHashtable(docs, docId, &kvp);
  if (result == 0)
    return (char*)kvp.value;
  else
    return NULL;
}
