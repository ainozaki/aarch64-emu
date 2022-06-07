#include "mem.h"

#include <stdio.h>
#include <stdlib.h>

#include <cassert>
#include <cstdint>

#include "arm.h"
#include "log.h"
#include "system.h"

namespace core {

namespace mem {

const uint32_t text_section_size = 1024 * 1024 * 1024; /// 1MB
const uint32_t mem_size = 1024 * 1024 * 1024;          /// 1MB

Mem::Mem(System *system) : system_(system) {}

void Mem::clean_mem() {
  free(text_);
  free(mem_);
  printf("mem: free mainmem\n");
}

int Mem::init_mem(const char *rawfile) {
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
  printf("mem: load file %s, size 0x%lx\n", rawfile, readlen);
  fclose(fp);

  system_->cpu().pc = (uint64_t)text_;
  printf("mem: set initial PC\n");
  printf("mem: PC = %p\n", text_);

  /// mem
  mem_ = (uint8_t *)malloc(mem_size);
  printf("mem: malloc mainmem size 0x%x  %p - %p\n", mem_size, mem_,
         mem_ + mem_size - 1);
  system_->cpu().xregs[31] = (uint64_t)(mem_size - 1024);
  printf("mem: set initial SP\n");
  printf("mem: SP = 0x%lx\n", system_->cpu().xregs[31]);

  return 0;
}

void *Mem::get_ptr(uint64_t vaddr) {
  if (vaddr >= mem_size) {
    fprintf(stderr, "=====Warning:memory access out of range=====\n");
    return NULL;
  }
  return vaddr + mem_;
}

void Mem::write(uint8_t size, uint64_t addr, uint64_t value) {
  switch (size) {
  case 0:
    return write_8(addr, value);
  case 1:
    return write_16(addr, value);
  case 2:
    return write_32(addr, value);
  case 3:
    return write_64(addr, value);
  default:
    assert(false);
  }
}

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

uint64_t Mem::read(uint8_t size, uint64_t addr) {
  void *paddr;

  paddr = get_ptr(addr);
  if (!paddr) {
    return 0;
  }
  LOG_CPU("mem: read: vaddr=0x%lx paddr=0x%p\n", addr, paddr);
  switch (size) {
  case 0:
    return read_8(paddr);
  case 1:
    return read_16(paddr);
  case 2:
    return read_32(paddr);
  case 3:
    return read_64(paddr);
  default:
    assert(false);
  }
}

uint8_t Mem::read_8(void *addr) { return *(uint8_t *)addr; }

uint16_t Mem::read_16(void *addr) { return *(uint16_t *)addr; }

uint32_t Mem::read_32(void *addr) { return *(uint32_t *)addr; }

uint64_t Mem::read_64(void *addr) { return *(uint64_t *)addr; }

} // namespace mem

} // namespace core
