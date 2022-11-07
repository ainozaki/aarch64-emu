#pragma once

#include <cstdint>

#include "mem.h"

enum class MemAccessSize {
  Byte,
  Hex,
  Word,
  DWord,
};

const MemAccessSize memsz_tbl[] = {
    MemAccessSize::Byte,
    MemAccessSize::Hex,
    MemAccessSize::Word,
    MemAccessSize::DWord,
};

class Bus {
public:
  Bus() = default;
  ~Bus() = default;

  Mem mem;

  uint64_t load(uint64_t address, MemAccessSize size);
  void store(uint64_t address, uint64_t value, MemAccessSize size);

  void init(uint64_t text_start, uint64_t text_size, uint64_t map_base);
};
