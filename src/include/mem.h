#pragma once

#include <cstdint>

#include "const.h"

class Mem
{
public:
  uint8_t *mem_;
  uint8_t *text_;
  uint8_t *text_end;

  Mem() = default;
  ~Mem() = default;

  void clean_mem();
  uint64_t get_ptr(uint64_t vaddr);

  uint8_t load8(uint64_t addr);
  uint16_t load16(uint64_t addr);
  uint32_t load32(uint64_t addr);
  uint64_t load64(uint64_t addr);
  void store8(uint64_t addr, uint8_t value);
  void store16(uint64_t addr, uint16_t value);
  void store32(uint64_t addr, uint32_t value);
  void store64(uint64_t addr, uint64_t value);

private:
  void show_stack(uint64_t sp);

  uint64_t key;
  bool no_text = false;
};
