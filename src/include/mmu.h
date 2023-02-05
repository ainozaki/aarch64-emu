#pragma once

#include <cstdint>

#include "bus.h"

class MMU {
public:
  MMU() = default;
  ~MMU() = default;

  void init(Bus *bus, uint64_t *current_el);

  // TTBR0_EL1 / TTBR1_EL1 (Translation Table Register)
  // base address of translation table
  // TTBR0 is selected when the upper bits of vaddr is 0
  // TTBR1 is selected when the upper bits of vaddr is 1
  uint64_t ttbr0_el1;
  uint64_t ttbr1_el1;

  // STCLR (System Control Register)
  // enable/disable mmu
  uint64_t sctlr_el1 = 0xc50838;

  // TCR_ELn (Translation Control Register)
  // controls memory management features
  //   63                                                     34      32
  //  +------------------------------------------------------+----------+
  //  |                                                      | IPA size |
  //  +-----------------------------------------------------------------+
  //  | TG1|                |  T1SZ  | TG0 |                    | T0SZ  |
  //  +----+----------------+--------+-----+--------------------+-------+
  //   31 30                 21    16  15 14                     5      0
  // - IPA(Intermediate Physical Address) size
  //     - the maximum address size
  //     - 000->32bits, 101->48bits
  // - TG0/TG1 (Translation Granule)
  //     - the granule size
  //     - the smallest block of memory that can be mapped in translation table
  //     - TG0: 00->4KB, 01->64KB, 10->16KB
  //     - TG1: 01->16KB, 10->4KB, 11->64KB
  // - T0SZ/T1SZ
  //     - the number of the MSBs that must be ethier 0 or 1
  //     - memory region size is 2**(64-T0SZ)
  //     - virtual address size control the starting level of address
  //     translation.
  struct tcr_el1 {
    uint64_t value;
    uint8_t get_ipa() { return (value >> 32) & 0x7; }
    uint8_t get_tg1() { return (value >> 30) & 0x3; }
    uint8_t get_tg0() { return (value >> 14) & 0x3; }
    uint8_t get_t1sz() { return (value >> 16) & 0x3f; }
    uint8_t get_t0sz() { return value & 0x3f; }
    uint8_t get_max_addrsz() {
      switch (get_ipa()) {
      case 0:
        return 32;
      case 1:
        return 36;
      case 2:
        return 40;
      case 3:
        return 42;
      case 4:
        return 44;
      case 5:
        return 48;
      case 6:
        return 52;
      }
      return 0;
    }
    /*
    uint8_t buff1[29];
    uint8_t ipa_size[3];
    uint8_t tg1[2];
    uint8_t buff2[8];
    uint8_t t1sz[6];
    uint8_t tg0[2];
    uint8_t buff3[8];
    uint8_t t0sz[6];
    */
  } tcr_el1;

  uint64_t mmu_translate(uint64_t vaddr);
  bool if_mmu_enabled() { return sctlr_el1 & 1; }

private:
  Bus *bus_;
  uint64_t *current_el_;

  void mmu_debug(uint64_t addr);

  uint64_t l0_translate(uint64_t value, uint64_t base);
  uint64_t l1_translate(uint64_t value, uint64_t base);
  uint64_t l2_translate(uint64_t value, uint64_t base);
  uint64_t l3_translate(uint64_t value, uint64_t base);
};