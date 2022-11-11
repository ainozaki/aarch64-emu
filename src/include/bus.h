#pragma once

#include <cstdint>

#include "mem.h"

// GIC v3
const uint64_t gicv3_base = 0x08000000;
const uint64_t gicv3_size = 0x10000;

// UART
const uint64_t uart_base = 0x09000000;
const uint64_t uart_size = 0x1000;

// virtio mmio
const uint64_t virtio_mmio_base = 0x0a000000;
const uint64_t virtio_mmio_size = 0x200;

// RAM
const uint64_t ram_base = 0x40000000;
const uint64_t ram_size = 1024 * 1024 * 1024;

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
