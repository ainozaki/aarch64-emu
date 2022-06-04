#pragma once

#include <cstdint>

class Cpu;

class Mem {
public:
  Mem() = default;
  ~Mem() = default;

  int init_mem(const char *rawfile, Cpu &cpu);
  void clean_mem();

  void *get_ptr(uint64_t vaddr);
  void write_8(uint64_t addr, uint8_t value);
  uint8_t read_8(uint64_t addr);

private:
  uint8_t *mem_;
  uint8_t *text_;
};
