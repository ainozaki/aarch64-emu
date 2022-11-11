#pragma once

#include <cstdint>

#include "bus.h"

class MMU {
public:
  MMU() = default;
  ~MMU() = default;

  void init(Bus *bus);

  uint64_t ttbr0_el1; // translation table register
  uint64_t ttbr1_el1;
  uint64_t sctlr_el1 = 0; // system control register

  uint64_t mmu_translate(uint64_t vaddr);

private:
  Bus *bus_;

  bool if_mmu_enabled() { return sctlr_el1 & 1; }
};