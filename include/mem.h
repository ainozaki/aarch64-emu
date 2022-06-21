#pragma once

#include <cstdint>

#include "arm.h"
#include "const.h"

namespace core {

class System;

namespace mem {

class Mem {
public:
  Mem(System *system);
  ~Mem() = default;

	SystemResult init_mem(const char *rawfile);
  void clean_mem();

  void *get_ptr(uint64_t vaddr);

  void write(uint8_t size, uint64_t addr, uint64_t value);
  uint64_t read(uint8_t size, uint64_t addr);
	uint32_t read_inst(uint64_t pc);

  uint8_t *mem_;
  uint8_t *text_;
  uint8_t *text_end;

private:
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
