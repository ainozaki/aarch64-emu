#include "mem.h"

#include <stdio.h>
#include <stdlib.h>

#include <cassert>
#include <cmath>
#include <cstdint>

#include "log.h"

void Mem::clean_mem() {
  if (!no_text) {
    free(text_);
  }
  free(mem_);
  printf("mem: free mainmem\n");
}

void Mem::show_stack(uint64_t sp) {
  uint64_t addr;
  LOG_DEBUG("\t\t===============\n");
  for (int i = -15; i < 16; i++) {
    addr = sp + i * 4;
    LOG_DEBUG("\t[0x%lx]   0x%08x\n", addr, load32(get_ptr(addr)));

    LOG_DEBUG("\t\t===============\n");
  }
}

uint64_t Mem::get_ptr(uint64_t vaddr) {
  /*
  if (vaddr >= mem_size)
  {
    fprintf(stderr,
            "=====Warning:memory access out of range vaddr=0x%lx=====\n",
            vaddr);
    return 0;
  }
  return (uint64_t)(vaddr + mem_);
  */
  return vaddr;
}

void Mem::store8(uint64_t addr, const uint8_t value) {
  uint8_t *p = (uint8_t *)get_ptr(addr);
  p[0] = (uint8_t)value;
}

void Mem::store16(uint64_t addr, const uint16_t value) {
  uint8_t *p;
  if (!(p = (uint8_t *)get_ptr(addr))) {
    printf("cannot access to 0x%lx\n", addr);
    return;
  }
  p[0] = value;
  p[1] = (uint8_t)(value >> 8);
}

void Mem::store32(uint64_t addr, const uint32_t value) {
  uint8_t *p;
  if (!(p = (uint8_t *)get_ptr(addr))) {
    printf("cannot access to 0x%lx\n", addr);
    return;
  }
  p[0] = value;
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
}

void Mem::store64(uint64_t addr, const uint64_t value) {
  uint8_t *p;
  if (!(p = (uint8_t *)get_ptr(addr))) {
    printf("cannot access to 0x%lx\n", addr);
    return;
  }
  p[0] = value;
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
  p[4] = (uint8_t)(value >> 32);
  p[5] = (uint8_t)(value >> 40);
  p[6] = (uint8_t)(value >> 48);
  p[7] = (uint8_t)(value >> 56);
}

uint8_t Mem::load8(uint64_t addr) {
  uint8_t *p;
  if (!(p = (uint8_t *)get_ptr(addr))) {
    printf("cannot access to 0x%lx\n", addr);
    return 0;
  }
  return *(uint8_t *)p;
}

uint16_t Mem::load16(uint64_t addr) {
  uint8_t *p;
  if (!(p = (uint8_t *)get_ptr(addr))) {
    printf("cannot access to 0x%lx\n", addr);
    return 0;
  }
  return p[0] | p[1] << 8;
}

uint32_t Mem::load32(uint64_t addr) {
  uint8_t *p;
  if (!(p = (uint8_t *)get_ptr(addr))) {
    printf("cannot access to 0x%lx\n", addr);
    return 0;
  }
  return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24 | uint64_t(p[4]) << 32;
}

uint64_t Mem::load64(uint64_t addr) {
  uint8_t *p;
  if (!(p = (uint8_t *)get_ptr(addr))) {
    printf("cannot access to 0x%lx\n", addr);
    return 0;
  }
  return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24 | uint64_t(p[4]) << 32 |
         uint64_t(p[5]) << 40 | uint64_t(p[6]) << 48 | uint64_t(p[7]) << 56;
}
