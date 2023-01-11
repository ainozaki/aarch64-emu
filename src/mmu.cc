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
  // TCR debug
  LOG_EMU("========TCR========\n");
  LOG_EMU("ipa: 000->32bits, 101->48bits\n");
  LOG_EMU("\tipa = 0x%x\n", tcr_el1.get_ipa());
  LOG_EMU("tg1: 01->16KB, 10->4KB, 11->64KB\n");
  LOG_EMU("\ttg1 = 0x%x\n", tcr_el1.get_tg1());
  LOG_EMU("tg0: 00->4KB, 01->64KB, 10->16KB\n");
  LOG_EMU("\ttg0 = 0x%x\n", tcr_el1.get_tg0());
  LOG_EMU("tnsz: memory region size 2**(64-T0SZ)\n");
  LOG_EMU("\t64 - t1sz = %d\n", 64 - tcr_el1.get_t1sz());
  LOG_EMU("\t64 - t0sz = %d\n", 64 - tcr_el1.get_t0sz());

  // translation table debug
  LOG_EMU("========VADDR========\n");
  LOG_EMU("addr   = 0x%lx\n", addr);
  LOG_EMU("ttbrn   = 0x%x\n", util::shift(addr, g4kb_l0_start_bit + g4kb_index_sz, 63));
  LOG_EMU("L0_index = 0x%x\n", util::shift(addr, g4kb_l0_start_bit, g4kb_index_sz));
  LOG_EMU("L1_index = 0x%x\n", util::shift(addr, g4kb_l1_start_bit, g4kb_index_sz));
  LOG_EMU("L2_index = 0x%x\n", util::shift(addr, g4kb_l2_start_bit, g4kb_index_sz));
  LOG_EMU("L3_index = 0x%x\n", util::shift(addr, g4kb_l3_start_bit, g4kb_index_sz));
  LOG_EMU("offset  = 0x%lx\n", util::shift(addr, 0, 11));
  LOG_EMU("======================\n");
}

uint64_t MMU::l0_translate(uint64_t addr, uint64_t base) {
  uint64_t index, entry, next;

  LOG_EMU("L0\n");
  index = util::shift(addr, g4kb_l0_start_bit,
                      g4kb_l0_start_bit + g4kb_index_sz - 1);
  entry = bus_->load((uint64_t)(base + index * 8), MemAccessSize::DWord);

  LOG_EMU("\tbase  = 0x%lx\n", base);
  LOG_EMU("\tindex = 0x%ld\n", index);
  LOG_EMU("\tentry(*0x%lx) = 0x%lx\n", base + index * 8, entry);
  if (!(entry & 1)) {
    LOG_EMU("invalid entry\n");
    return 1;
  }

  assert(entry & 2);
  LOG_EMU("table entry\n");
  next = util::shift(entry, 12, 27) << 12;
  LOG_EMU("\tnext  = 0x%lx\n", next);
  return l1_translate(addr, next);
}

uint64_t MMU::l1_translate(uint64_t addr, uint64_t base) {
  uint64_t index, entry, next, output, offset;
  LOG_EMU("L1\n");
  index = util::shift(addr, g4kb_l1_start_bit,
                      g4kb_l1_start_bit + g4kb_index_sz - 1);
  entry = bus_->load((uint64_t)(base + index * 8), MemAccessSize::DWord);

  LOG_EMU("\tbase  = 0x%lx\n", base);
  LOG_EMU("\tindex = 0x%ld\n", index);
  LOG_EMU("\tentry(*0x%lx) = 0x%lx\n", base + index * 8, entry);
  if (!(entry & 1)) {
    LOG_EMU("invalid entry\n");
    return 1;
  }

  if (entry & 2) {
    LOG_EMU("\ttable entry\n");
    next = util::shift(entry, 12, 47) << 12;
    LOG_EMU("\tnext  = 0x%lx\n", next);
    return l2_translate(addr, next);
  } else {
    LOG_EMU("block entry\n");
    output = util::shift(entry, 30, 47) << 30;
    offset = util::shift(addr, 0, g4kb_l1_start_bit - 1);
    output |= offset;
    LOG_EMU("\toutput = 0x%lx\n", output);
    return output;
  }
}

uint64_t MMU::l2_translate(uint64_t addr, uint64_t base) {
  uint64_t index, entry, next, output, offset;
  LOG_EMU("L2\n");
  index = util::shift(addr, g4kb_l2_start_bit,
                      g4kb_l2_start_bit + g4kb_index_sz - 1);
  entry = bus_->load((uint64_t)(base + index * 8), MemAccessSize::DWord);

  LOG_EMU("\tbase  = 0x%lx\n", base);
  LOG_EMU("\tindex = 0x%ld\n", index);
  LOG_EMU("\tentry(*0x%lx) = 0x%lx\n", base + index * 8, entry);
  if (!(entry & 1)) {
    LOG_EMU("invalid entry\n");
    return 1;
  }

  if (entry & 2) {
    LOG_EMU("\ttable entry\n");
    next = util::shift(entry, 12, 47) << 12;
    LOG_EMU("\tnext  = 0x%lx\n", next);
    return l3_translate(addr, next);
  } else {
    LOG_EMU("\tblock entry\n");
    output = util::shift(entry, 21, 47) << 21;
    offset = util::shift(addr, 0, g4kb_l2_start_bit - 1);
    output |= offset;
    LOG_EMU("\toutput = 0x%lx\n", output);
    return output;
  }
}

uint64_t MMU::l3_translate(uint64_t addr, uint64_t base) {
  uint64_t index, entry, output, offset;
  LOG_EMU("L3\n");
  index = util::shift(addr, g4kb_l3_start_bit,
                      g4kb_l3_start_bit + g4kb_index_sz - 1);
  entry = bus_->load((uint64_t)(base + index * 8), MemAccessSize::DWord);

  LOG_EMU("\tbase  = 0x%lx\n", base);
  LOG_EMU("\tindex = 0x%ld\n", index);
  LOG_EMU("\tentry(*0x%lx) = 0x%lx\n", base + index * 8, entry);
  if (!(entry & 1)) {
    LOG_EMU("invalid entry\n");
    return 1;
  }

  assert(!(entry & 2));
  LOG_EMU("\tblock entry\n");
  output = util::shift(entry, 21, 47) << 21;
  offset = util::shift(entry, 0, g4kb_l0_start_bit - 1);
  output |= offset;
  LOG_EMU("\toutput = 0x%lx\n", output);
  return output;
}

uint64_t MMU::mmu_translate(uint64_t addr) {
  //    4kb granule
  //     63   48 47    39 38    30 29    21 20    12 11       0
  //    +-------+--------+--------+--------+--------+----------+
  //    | TTBRn | level0 | level1 | level2 | level3 | page off |
  //    |       |        | (PUD)  | (PMD)  | (PTE)  |          |
  //    +-------+--------+--------+--------+--------+----------+
  //
  uint8_t addr_sz;
  uint64_t ttbrn, msbs, paddr;

  if (!if_mmu_enabled()) {
    return addr;
  }

  addr_sz = current_el_ ? 64 - tcr_el1.get_t1sz() : 64 - tcr_el1.get_t0sz();
  addr_sz = std::min(addr_sz, tcr_el1.get_max_addrsz());
  msbs = util::shift(addr, addr_sz, 63);

  //mmu_debug(addr);
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

  LOG_EMU("paddr = 0x%lx\n", paddr);

  return paddr;
}