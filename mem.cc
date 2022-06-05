#include "mem.h"

#include <stdio.h>
#include <stdlib.h>

#include <cstdint>

#include <arm.h>

const uint32_t text_section_size = 1024 * 1024 * 1024; /// 1MB
const uint32_t mem_size = 1024 * 1024 * 1024;          /// 1MB

void Mem::clean_mem() {
  free(text_);
  free(mem_);
  printf("mem: free mainmem\n");
}

int Mem::init_mem(const char *rawfile, Cpu &cpu) {
  FILE *fp;
  size_t readlen;

  /// text
  text_ = (uint8_t *)calloc(1, text_section_size);
  fp = fopen(rawfile, "r");
  if (!fp) {
    fprintf(stderr, "mem: cannot open file %s\n", rawfile);
    free(text_);
    return -1;
  }
  readlen = fread(text_, 1, text_section_size, fp);
  printf("mem: load file %s, size %lu\n", rawfile, readlen);
  fclose(fp);

  cpu.pc = (uint64_t)text_;
  printf("mem: set initial PC\n");
  printf("mem: PC = %p\n", text_);

  /// mem
  mem_ = (uint8_t *)malloc(mem_size);
  printf("mem: malloc mainmem size %u at %p\n", mem_size, mem_);
  cpu.sp = (uint64_t)(mem_ + mem_size - 1);
  printf("mem: set initial SP\n");
  printf("mem: SP = %p\n", mem_ + mem_size);

  return 0;
}

void *Mem::get_ptr(uint64_t vaddr) { return vaddr + mem_; }

void Mem::write_8(uint64_t addr, uint8_t value) {
  void *paddr = get_ptr(addr);
  *(uint8_t *)paddr = value;
}

void Mem::write_16(uint64_t addr, uint16_t value) {
  void *paddr = get_ptr(addr);
  *(uint16_t *)paddr = value;
}

void Mem::write_32(uint64_t addr, uint32_t value) {
  void *paddr = get_ptr(addr);
  *(uint32_t *)paddr = value;
}

void Mem::write_64(uint64_t addr, uint64_t value) {
  void *paddr = get_ptr(addr);
  *(uint64_t *)paddr = value;
}

uint8_t Mem::read_8(uint64_t addr) { return *(uint8_t *)get_ptr(addr); }

uint16_t Mem::read_16(uint64_t addr) { return *(uint16_t *)get_ptr(addr); }

uint32_t Mem::read_32(uint64_t addr) { return *(uint32_t *)get_ptr(addr); }

uint64_t Mem::read_64(uint64_t addr) { return *(uint64_t *)get_ptr(addr); }
