#include "mem.h"

#include <stdio.h>
#include <stdlib.h>

#include <cassert>
#include <cmath>
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
  
	/// mem
  mem_ = (uint8_t *)malloc(mem_size);
  printf("mem: malloc mainmem size 0x%x  %p - %p\n", mem_size, mem_,
         mem_ + mem_size - 1);
  system_->cpu().xregs[31] = (uint64_t)(mem_size - 1024);
  printf("mem: set initial SP\n");
  printf("mem: SP = 0x%lx\n", system_->cpu().xregs[31]);

  /// text
  text_ = (uint8_t *)calloc(1, text_section_size);
  fp = fopen(rawfile, "r");
  if (!fp) {
    fprintf(stderr, "mem: cannot open file %s. Proceed without text code.\n", rawfile);
    free(text_);
    return -1;
  }
  readlen = fread(text_, 1, text_section_size, fp);
  text_end = text_ + readlen - 1;
  printf("mem: load file %s, size 0x%lx\n", rawfile, readlen);
  fclose(fp);

  system_->cpu().pc = (uint64_t)text_;
  printf("mem: set initial PC\n");
  printf("mem: PC = %p\n", text_);

  return 0;
}

void *Mem::get_ptr(uint64_t vaddr) {
  if (vaddr >= mem_size) {
    fprintf(stderr, "=====Warning:memory access out of range=====\n");
    return NULL;
  }
  return vaddr + mem_;
}

void Mem::write(uint8_t size, uint64_t vaddr, uint64_t value) {
  void *paddr;
  paddr = get_ptr(vaddr);
  if (!paddr) {
    return;
  }
  LOG_CPU("\tmem: write: vaddr=0x%lx paddr=0x%p\n", vaddr, paddr);
  switch (size) {
  case 0:
    return write_8(paddr, value);
  case 1:
    return write_16(paddr, value);
  case 2:
    return write_32(paddr, value);
  case 3:
    return write_64(paddr, value);
  default:
    assert(false);
  }
}

void Mem::write_8(void *paddr, const uint8_t value) {
  uint8_t *p = (uint8_t *)paddr;
  p[0] = (uint8_t)value;
}

void Mem::write_16(void *paddr, const uint16_t value) {
  uint8_t *p = (uint8_t *)paddr;
  p[0] = value;
  p[1] = (uint8_t)(value >> 8);
}

void Mem::write_32(void *paddr, const uint32_t value) {
  uint8_t *p = (uint8_t *)paddr;
  p[0] = value;
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
}

void Mem::write_64(void *paddr, const uint64_t value) {
  uint8_t *p = (uint8_t *)paddr;
  p[0] = value;
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
  p[4] = (uint8_t)(value >> 32);
  p[5] = (uint8_t)(value >> 40);
  p[6] = (uint8_t)(value >> 48);
  p[7] = (uint8_t)(value >> 56);
}

uint64_t Mem::read(uint8_t size, const uint64_t addr) {
  void *paddr;

  paddr = get_ptr(addr);
  if (!paddr) {
    return 0;
  }
  LOG_CPU("\tmem: read: vaddr=0x%lx paddr=0x%p\n", addr, paddr);
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
    return -1;
  }
}

uint32_t Mem::read_inst(uint64_t pc) {
  uint8_t *p = (uint8_t *)pc;
  return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24 | uint64_t(p[4]) << 32;
}

uint8_t Mem::read_8(const void *paddr) { return *(uint8_t *)paddr; }

uint16_t Mem::read_16(const void *paddr) {
  uint8_t *p = (uint8_t *)paddr;
  return p[0] | p[1] << 8;
}

uint32_t Mem::read_32(const void *paddr) {
  uint8_t *p = (uint8_t *)paddr;
  return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24 | uint64_t(p[4]) << 32;
}

uint64_t Mem::read_64(const void *paddr) {
  uint8_t *p = (uint8_t *)paddr;
  return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24 | uint64_t(p[4]) << 32 |
         uint64_t(p[5]) << 40 | uint64_t(p[6]) << 48 | uint64_t(p[7]) << 56;
}

} // namespace mem

} // namespace core
