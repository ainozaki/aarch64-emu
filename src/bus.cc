#include "bus.h"

#include <stdio.h>
#include <stdlib.h>

#include "mem.h"
#include "mmu.h"
#include "utils.h"

void Bus::init(uint64_t text_start, uint64_t text_size, uint64_t map_base) {
  mem.init(text_start, text_size, map_base);
}

uint64_t Bus::load(uint64_t address, MemAccessSize size) {
  switch (size) {
  case MemAccessSize::Byte:
    return mem.load8(address);
  case MemAccessSize::Hex:
    return mem.load16(address);
  case MemAccessSize::Word:
    return mem.load32(address);
  case MemAccessSize::DWord:
    return mem.load64(address);
  default:
    assert(false);
    // dummy
    return 0;
  }
}

void Bus::store(uint64_t address, uint64_t value, MemAccessSize size) {
  switch (size) {
  case MemAccessSize::Byte:
    mem.store8(address, value);
    break;
  case MemAccessSize::Hex:
    mem.store16(address, value);
    break;
  case MemAccessSize::Word:
    mem.store32(address, value);
    break;
  case MemAccessSize::DWord:
    mem.store64(address, value);
    break;
  default:
    assert(false);
  }
}