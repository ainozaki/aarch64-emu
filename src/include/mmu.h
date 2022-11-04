#pragma once

#include <cstdint>

enum class MMUregister {
  ttbr0_el1,
  ttbr1_el1,
};

class MMU {
public:
  MMU() = default;
  ~MMU() = default;

  uint64_t ttbr0_el1; // translation table register
  uint64_t ttbr1_el1;
  uint64_t sctlr_el1 = 0; // system control register

  uint64_t mmu_translate(uint64_t vaddr);

  void mmu_set_register(MMUregister type, uint64_t value);
  uint64_t mmu_get_register(MMUregister type);

private:
  bool if_mmu_enabled() { return sctlr_el1 & 1; }
};