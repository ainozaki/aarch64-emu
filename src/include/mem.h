#pragma once

#include <cstdint>

#include "arm.h"
#include "const.h"

namespace core {

class System;

namespace mem {

enum class MemAccess {
  Size8,
  Size16,
  Size32,
  Size64,
};

class Mem {
public:
  Mem(System *system);
  ~Mem() = default;

  SystemResult init_mem(const char *rawfile, const uint64_t initaddr);
  void clean_mem();

  void *get_ptr(uint64_t vaddr);

  void write(MemAccess size, uint64_t addr, uint64_t value);
  uint64_t read(MemAccess size, uint64_t addr);
  uint32_t read_inst(uint64_t pc);

  uint8_t *mem_;
  uint8_t *text_;
  uint8_t *text_end;

private:
  void show_stack();

  void write_8(void *addr, const uint8_t value);
  void write_16(void *addr, const uint16_t value);
  void write_32(void *addr, const uint32_t value);
  void write_64(void *addr, const uint64_t value);
  uint8_t read_8(const void *addr);
  uint16_t read_16(const void *addr);
  uint32_t read_32(const void *addr);
  uint64_t read_64(const void *addr);

  uint64_t key;
  bool no_text = false;

  System *system_;
};

} // namespace mem

} // namespace core
