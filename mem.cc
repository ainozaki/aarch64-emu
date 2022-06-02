#include "mem.h"

#include <stdio.h>
#include <stdlib.h>

#include <cstdint>

Mem::~Mem() {
  free(mem_);
  printf("mem: free mainmem\n");
}

void Mem::initialize_mainmem(const char *file, uint64_t load_offset) {
  FILE *fp;
  size_t readlen;

  mem_ = (uint8_t *)calloc(1, size);
  printf("mem: calloc mainmem size %lu\n", size);

  fp = fopen(file, "r");
  if (!fp) {
    fprintf(stderr, "mem: cannot open file %s\n", file);
    return;
  }
  readlen = fread(mem_ + load_offset, 1, size, fp);
  fclose(fp);
  printf("mem: load file %s, size %lu\n", file, readlen);
}
