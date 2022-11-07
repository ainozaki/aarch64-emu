#pragma once

#include <cstdint>

const uint64_t uart_base = 0x09000000;
const uint64_t uart_size = 0x00001000;

class MMU {
public:
  MMU() = default;
  ~MMU() = default;

  uint64_t ttbr0_el1; // translation table register
  uint64_t ttbr1_el1;
  uint64_t sctlr_el1 = 0; // system control register

  uint64_t mmu_translate(uint64_t vaddr);

private:
  bool if_mmu_enabled() { return sctlr_el1 & 1; }
};