#pragma once

#include <cstdint>

#include "mem.h"

class Bus {
public:
  Bus() = default;
  ~Bus() = default;

  Mem mem;

  void init(uint64_t text_start, uint64_t text_size, uint64_t map_base);
  uint8_t load8(uint64_t address);
  uint16_t load16(uint64_t address);
  uint32_t load32(uint64_t address);
  uint64_t load64(uint64_t address);
  void store8(uint64_t address, uint8_t value);
  void store16(uint64_t address, uint16_t value);
  void store32(uint64_t address, uint32_t value);
  void store64(uint64_t address, uint64_t value);
};
