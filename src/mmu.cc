#include "mmu.h"

#include <cassert>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
  uint64_t *index;
  ttbrn = util::shift(addr, 39, 63);
  pud = util::shift(addr, 30, 38);
  pmd = util::shift(addr, 21, 29);
  pte = util::shift(addr, 12, 20);
  offset = util::shift(addr, 0, 11);
  printf("======================\n");
  printf("addr   = 0x%lx\n", addr);
  printf("ttbrn   = 0x%x\n", ttbrn);
  printf("L1(pud) = 0x%x\n", pud);
  printf("L2(pmd) = 0x%x\n", pmd);
  printf("L3(pte) = 0x%x\n", pte);
  printf("offset  = 0x%x\n", offset);
  printf("======================\n");
  if (ttbrn == 0) {
    printf("ttbr0_el1 = 0x%lx\n", ttbr0_el1);
    index = (uint64_t *)ttbr0_el1;
    index += pud;
    printf("ttbr0_el1 + pud = 0x%lx\n", (uint64_t)index);
  } else {
    printf("ttbr1_el1 = 0x%lx\n", ttbr1_el1);
  }
  printf("======================\n");
  exit(0);
  return addr;
}