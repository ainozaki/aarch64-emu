#include "mmu.h"

#include <cassert>

#include <stdio.h>

void MMU::mmu_set_register(MMUregister type, uint64_t value) {
  switch (type) {
  case MMUregister::ttbr0_el1:
    ttbr0_el1 = value;
    break;
  case MMUregister::ttbr1_el1:
    ttbr1_el1 = value;
    break;
  default:
    assert(false);
  }
}

uint64_t MMU::mmu_get_register(MMUregister type) {
  switch (type) {
  case MMUregister::ttbr0_el1:
    return ttbr0_el1;
    break;
  case MMUregister::ttbr1_el1:
    return ttbr1_el1;
    break;
  default:
    assert(false);
    return 0;
  }
}