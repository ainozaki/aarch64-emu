#include "mmu.h"

#include <cassert>

#include <stdio.h>

#include "utils.h"

uint64_t MMU::mmu_translate(uint64_t addr) {
  //    vaddr
  //     63   39 38    30 29    21 20    12 11       0
  //    +-------+--------+--------+--------+----------+
  //    | TTBRn | level1 | level2 | level3 | page off |
  //    |       | (PUD)  | (PMD)  | (PTE)  |          |
  //    +-------+--------+--------+--------+----------+
  //
  uint32_t ttbrn;
  uint16_t pud, pmd, pte, offset;

  if (!if_mmu_enabled()) {
    return addr;
  }

  ttbrn = util::shift(addr, 39, 63);
  pud = util::shift(addr, 30, 38);
  pmd = util::shift(addr, 21, 29);
  pte = util::shift(addr, 12, 20);
  offset = util::shift(addr, 0, 11);
  printf("addr   = 0x%lx\n", addr);
  printf("ttbrn   = 0x%x\n", ttbrn);
  printf("pud     = 0x%x\n", pud);
  printf("pmd     = 0x%x\n", pmd);
  printf("pte     = 0x%x\n", pte);
  printf("offset  = 0x%x\n", offset);
  return addr;
}

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