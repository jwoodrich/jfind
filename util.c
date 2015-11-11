#include "util.h"
#include <stdlib.h>
#include <stdio.h>

void *calloc_or_die(char *name, size_t nitems, size_t size) {
  void *ret=calloc(nitems,size);
  if (ret==NULL) {
    fprintf(stdout,"failed to allocate memory for %d items of %d bytes for %s!\n",(int)nitems,(int)size,name);
    exit(2);
  }
  return ret;
}

