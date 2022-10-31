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

  uint64_t ttbr0_el1;
  uint64_t ttbr1_el1;

  void mmu_set_register(MMUregister type, uint64_t value);
  uint64_t mmu_get_register(MMUregister type);
};