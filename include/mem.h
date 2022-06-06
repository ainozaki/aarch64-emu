#pragma once

#include <cstdint>

#include "arm.h"

namespace core {

class System;

namespace mem {

class Mem {
public:
  Mem(System *system);
  ~Mem() = default;

  int init_mem(const char *rawfile);
  void clean_mem();

  void *get_ptr(uint64_t vaddr);

  void write(uint8_t size, uint64_t addr, uint64_t value);
  uint64_t read(uint8_t size, uint64_t addr);

private:
  void write_8(uint64_t addr, uint8_t value);
  void write_16(uint64_t addr, uint16_t value);
  void write_32(uint64_t addr, uint32_t value);
  void write_64(uint64_t addr, uint64_t value);
  uint8_t read_8(uint64_t addr);
  uint16_t read_16(uint64_t addr);
  uint32_t read_32(uint64_t addr);
  uint64_t read_64(uint64_t addr);

  uint8_t *mem_;
  uint8_t *text_;

  System *system_;
};

} // namespace mem

} // namespace core
