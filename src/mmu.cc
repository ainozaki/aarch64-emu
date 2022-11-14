#include "mmu.h"

#include <cassert>
#include <cmath>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"

const uint8_t g4kb_index_sz = 9;
const uint8_t g4kb_l0_start_bit = 39;
const uint8_t g4kb_l1_start_bit = 30;
const uint8_t g4kb_l2_start_bit = 21;
const uint8_t g4kb_l3_start_bit = 12;

void MMU::init(Bus *bus, uint64_t *current_el) {
  assert(bus);
  bus_ = bus;
  current_el_ = current_el;
}

void MMU::mmu_debug(uint64_t addr) {
  uint32_t ttbrn;
  uint16_t pud, pmd, pte;
  uint64_t offset;
  ttbrn = util::shift(addr, 39, 63);
  pud = util::shift(addr, 30, 38);
  pmd = util::shift(addr, 21, 29);
  pte = util::shift(addr, 12, 20);
  offset = util::shift(addr, 0, 11);

  // TCR debug
  printf("========TCR========\n");
  printf("ipa: 000->32bits, 101->48bits\n");
  printf("\tipa = 0x%x\n", tcr_el1.get_ipa());
  printf("tg1: 01->16KB, 10->4KB, 11->64KB\n");
  printf("\ttg1 = 0x%x\n", tcr_el1.get_tg1());
  printf("tg0: 00->4KB, 01->64KB, 10->16KB\n");
  printf("\ttg0 = 0x%x\n", tcr_el1.get_tg0());
  printf("tnsz: memory region size 2**(64-T0SZ)\n");
  printf("\t64 - t1sz = %d\n", 64 - tcr_el1.get_t1sz());
  printf("\t64 - t0sz = %d\n", 64 - tcr_el1.get_t0sz());

  // translation table debug
  printf("========VADDR========\n");
  printf("addr   = 0x%lx\n", addr);
  printf("ttbrn   = 0x%x\n", ttbrn);
  printf("L1(pud) = 0x%x\n", pud);
  printf("L2(pmd) = 0x%x\n", pmd);
  printf("L3(pte) = 0x%x\n", pte);
  printf("offset  = 0x%lx\n", offset);
  printf("======================\n");
}

uint64_t MMU::l0_translate(uint64_t addr, uint64_t base) {
  uint64_t index, entry, next;

  printf("L0\n");
  index = util::shift(addr, g4kb_l0_start_bit,
                      g4kb_l0_start_bit + g4kb_index_sz - 1);
  entry = bus_->load((uint64_t)(base + index * 8), MemAccessSize::DWord);

  printf("\tbase  = 0x%lx\n", base);
  printf("\tindex = 0x%ld\n", index);
  printf("\tentry(*0x%lx) = 0x%lx\n", base + index * 8, entry);
  if (!(entry & 1)) {
    printf("invalid entry\n");
    return 1;
  }

  assert(entry & 2);
  printf("table entry\n");
  next = util::shift(entry, 12, 27) << 12;
  printf("\tnext  = 0x%lx\n", next);
  return l1_translate(addr, next);
}

uint64_t MMU::l1_translate(uint64_t addr, uint64_t base) {
  uint64_t index, entry, next, output;
  printf("L1\n");
  index = util::shift(addr, g4kb_l1_start_bit,
                      g4kb_l1_start_bit + g4kb_index_sz - 1);
  entry = bus_->load((uint64_t)(base + index * 8), MemAccessSize::DWord);

  printf("\tbase  = 0x%lx\n", base);
  printf("\tindex = 0x%ld\n", index);
  printf("\tentry(*0x%lx) = 0x%lx\n", base + index * 8, entry);
  if (!(entry & 1)) {
    printf("invalid entry\n");
    return 1;
  }

  if (entry & 2) {
    printf("\ttable entry\n");
    next = util::shift(entry, 12, 47) << 12;
    printf("\tnext  = 0x%lx\n", next);
    return l2_translate(addr, next);
  } else {
    printf("block entry\n");
    output = util::shift(entry, 30, 47) << 30;
    printf("\toutput = 0x%lx\n", output);
    return output;
  }
}

uint64_t MMU::l2_translate(uint64_t addr, uint64_t base) {
  uint64_t index, entry, next, output;
  printf("L2\n");
  index = util::shift(addr, g4kb_l2_start_bit,
                      g4kb_l2_start_bit + g4kb_index_sz - 1);
  entry = bus_->load((uint64_t)(base + index * 8), MemAccessSize::DWord);

  printf("\tbase  = 0x%lx\n", base);
  printf("\tindex = 0x%ld\n", index);
  printf("\tentry(*0x%lx) = 0x%lx\n", base + index * 8, entry);
  if (!(entry & 1)) {
    printf("invalid entry\n");
    return 1;
  }

  if (entry & 2) {
    printf("\ttable entry\n");
    next = util::shift(entry, 12, 47) << 12;
    printf("\tnext  = 0x%lx\n", next);
    return l3_translate(addr, next);
  } else {
    printf("\tblock entry\n");
    output = util::shift(entry, 21, 47) << 21;
    printf("\toutput = 0x%lx\n", output);
    return output;
  }
}

uint64_t MMU::l3_translate(uint64_t addr, uint64_t base) {
  uint64_t index, entry, output;
  printf("L3\n");
  index = util::shift(addr, g4kb_l3_start_bit,
                      g4kb_l3_start_bit + g4kb_index_sz - 1);
  entry = bus_->load((uint64_t)(base + index * 8), MemAccessSize::DWord);

  printf("\tbase  = 0x%lx\n", base);
  printf("\tindex = 0x%ld\n", index);
  printf("\tentry(*0x%lx) = 0x%lx\n", base + index * 8, entry);
  if (!(entry & 1)) {
    printf("invalid entry\n");
    return 1;
  }

  assert(!(entry & 2));
  printf("\tblock entry\n");
  output = util::shift(entry, 21, 47) << 21;
  printf("\toutput = 0x%lx\n", output);
  return output;
}

uint64_t MMU::mmu_translate(uint64_t addr) {
  //    vaddr
  //     63   48 47    39 38    30 29    21 20    12 11       0
  //    +-------+--------+--------+--------+--------+----------+
  //    | TTBRn | level0 | level1 | level2 | level3 | page off |
  //    |       |        | (PUD)  | (PMD)  | (PTE)  |          |
  //    +-------+--------+--------+--------+--------+----------+
  //
  uint8_t addr_sz;
  uint64_t offset, ttbrn, msbs, paddr;

  if (!if_mmu_enabled()) {
    return addr;
  }

  addr_sz = current_el_ ? 64 - tcr_el1.get_t1sz() : 64 - tcr_el1.get_t0sz();
  addr_sz = std::min(addr_sz, tcr_el1.get_max_addrsz());
  msbs = util::shift(addr, addr_sz, 63);
  offset = util::shift64(addr, 0, 11);

  // mmu_debug(addr);
  ttbrn = msbs ? ttbr1_el1 : ttbr0_el1;

  if (addr_sz >= g4kb_l0_start_bit) {
    paddr = l0_translate(addr, ttbrn);
  } else if (addr_sz >= g4kb_l1_start_bit) {
    paddr = l1_translate(addr, ttbrn);
  } else if (addr_sz >= g4kb_l2_start_bit) {
    paddr = l2_translate(addr, ttbrn);
  } else if (addr_sz >= g4kb_l3_start_bit) {
    paddr = l3_translate(addr, ttbrn);
  }

  paddr = paddr | offset;
  printf("paddr = 0x%lx\n", paddr);

  return paddr;
}