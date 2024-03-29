#include "mmu.h"

#include <cassert>
#include <cmath>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"
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

void MMU::mmu_debug([[maybe_unused]] uint64_t addr) {
  // TCR debug
  LOG_DEBUG("========TCR========\n");
  LOG_DEBUG("ipa: 000->32bits, 101->48bits\n");
  LOG_DEBUG("\tipa = 0x%x\n", tcr_el1.get_ipa());
  LOG_DEBUG("tg1: 01->16KB, 10->4KB, 11->64KB\n");
  LOG_DEBUG("\ttg1 = 0x%x\n", tcr_el1.get_tg1());
  LOG_DEBUG("tg0: 00->4KB, 01->64KB, 10->16KB\n");
  LOG_DEBUG("\ttg0 = 0x%x\n", tcr_el1.get_tg0());
  LOG_DEBUG("tnsz: memory region size 2**(64-T0SZ)\n");
  LOG_DEBUG("\t64 - t1sz = %d\n", 64 - tcr_el1.get_t1sz());
  LOG_DEBUG("\t64 - t0sz = %d\n", 64 - tcr_el1.get_t0sz());

  // translation table debug
  LOG_DEBUG("========VADDR========\n");
  LOG_DEBUG("addr   = 0x%lx\n", addr);
  LOG_DEBUG("ttbrn   = 0x%lx\n", util::bit64(addr, 63));
  LOG_DEBUG("L0_index = 0x%lx\n",
            util::shift(addr, g4kb_l0_start_bit, g4kb_l0_start_bit + 8));
  LOG_DEBUG("L1_index = 0x%lx\n",
            util::shift(addr, g4kb_l1_start_bit, g4kb_l1_start_bit + 8));
  LOG_DEBUG("L2_index = 0x%lx\n",
            util::shift(addr, g4kb_l2_start_bit, g4kb_l2_start_bit + 8));
  LOG_DEBUG("L3_index = 0x%lx\n",
            util::shift(addr, g4kb_l3_start_bit, g4kb_l3_start_bit + 8));
  LOG_DEBUG("offset  = 0x%lx\n", util::shift(addr, 0, 11));
  LOG_DEBUG("ttbr0  = 0x%lx\n", ttbr0_el1);
  LOG_DEBUG("ttbr1  = 0x%lx\n", ttbr1_el1);
  LOG_DEBUG("======================\n");
}

uint64_t MMU::l0_translate(uint64_t addr, uint64_t base) {
  uint64_t index, entry, next;

  LOG_DEBUG("L0\n");
  index = util::shift(addr, g4kb_l0_start_bit, g4kb_l0_start_bit + 8);
  entry = bus_->load((uint64_t)(base + index * 8), MemAccessSize::DWord);

  LOG_DEBUG("\tbase  = 0x%lx\n", base);
  LOG_DEBUG("\tindex = 0x%ld\n", index);
  LOG_DEBUG("\tentry(*0x%lx) = 0x%lx\n", base + index * 8, entry);
  if (!(entry & 1)) {
    LOG_DEBUG("invalid entry\n");
    return 1;
  }

  assert(entry & 2);
  LOG_DEBUG("table entry\n");
  next = util::shift(entry, 12, 27) << 12;
  LOG_DEBUG("\tnext  = 0x%lx\n", next);
  return l1_translate(addr, next);
}

uint64_t MMU::l1_translate(uint64_t addr, uint64_t base) {
  uint64_t index, entry, next, output, offset;
  LOG_DEBUG("L1\n");
  index = util::shift(addr, g4kb_l1_start_bit, g4kb_l1_start_bit + 8);
  entry = bus_->load((uint64_t)(base + index * 8), MemAccessSize::DWord);

  LOG_DEBUG("\tbase  = 0x%lx\n", base);
  LOG_DEBUG("\tindex = 0x%ld\n", index);
  LOG_DEBUG("\tentry(*0x%lx) = 0x%lx\n", base + index * 8, entry);
  if (!(entry & 1)) {
    LOG_DEBUG("invalid entry\n");
    return 1;
  }

  if (entry & 2) {
    LOG_DEBUG("\ttable entry\n");
    next = util::shift(entry, 12, 47) << 12;
    LOG_DEBUG("\tnext  = 0x%lx\n", next);
    return l2_translate(addr, next);
  } else {
    LOG_DEBUG("block entry\n");
    output = util::shift(entry, 30, 47) << 30;
    offset = util::shift(addr, 0, g4kb_l1_start_bit - 1);
    output |= offset;
    LOG_DEBUG("\toutput = 0x%lx\n", output);
    return output;
  }
}

uint64_t MMU::l2_translate(uint64_t addr, uint64_t base) {
  uint64_t index, entry, next, output, offset;
  LOG_DEBUG("L2\n");
  index = util::shift(addr, g4kb_l2_start_bit, g4kb_l2_start_bit + 8);
  entry = bus_->load((uint64_t)(base + index * 8), MemAccessSize::DWord);

  LOG_DEBUG("\tbase  = 0x%lx\n", base);
  LOG_DEBUG("\tindex = 0x%ld\n", index);
  LOG_DEBUG("\tentry(*0x%lx) = 0x%lx\n", base + index * 8, entry);
  if (!(entry & 1)) {
    LOG_DEBUG("invalid entry\n");
    return 1;
  }

  if (entry & 2) {
    LOG_DEBUG("\ttable entry\n");
    next = util::shift(entry, 12, 47) << 12;
    LOG_DEBUG("\tnext  = 0x%lx\n", next);
    return l3_translate(addr, next);
  } else {
    LOG_DEBUG("\tblock entry\n");
    output = util::shift(entry, 21, 47) << 21;
    offset = util::shift(addr, 0, g4kb_l2_start_bit - 1);
    output |= offset;
    LOG_DEBUG("\toutput = 0x%lx\n", output);
    return output;
  }
}

uint64_t MMU::l3_translate(uint64_t addr, uint64_t base) {
  uint64_t index, entry, output, offset;
  LOG_DEBUG("L3\n");
  index = util::shift(addr, g4kb_l3_start_bit, g4kb_l3_start_bit + 8);
  entry = bus_->load((uint64_t)(base + index * 8), MemAccessSize::DWord);

  LOG_DEBUG("\tbase  = 0x%lx\n", base);
  LOG_DEBUG("\tindex = 0x%ld\n", index);
  LOG_DEBUG("\tentry(*0x%lx) = 0x%lx\n", base + index * 8, entry);
  if (!(entry & 1)) {
    LOG_DEBUG("invalid entry\n");
    return 1;
  }

  assert(!(entry & 2));
  LOG_DEBUG("\tblock entry\n");
  output = util::shift(entry, 12, 47) << 12;
  offset = util::shift(addr, 0, g4kb_l3_start_bit - 1);
  output |= offset;
  LOG_DEBUG("\toutput = 0x%lx\n", output);
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

  msbs = util::bit64(addr, 63);
  addr_sz = msbs ? 64 - tcr_el1.get_t1sz() : 64 - tcr_el1.get_t0sz();
  addr_sz = std::min(addr_sz, tcr_el1.get_max_addrsz());

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
  mmu_debug(addr);

  LOG_DEBUG("vaddr = 0x%lx, paddr = 0x%lx\n", addr, paddr);

  return paddr;
}