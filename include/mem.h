#pragma once

#include <cstdint>

class Mem {
public:
  Mem() = default;
  ~Mem();
  void initialize_mainmem(const char *file, const uint64_t load_offset);

private:
  const uint64_t size = 4 * 1024 * 1024;
  const uint64_t base = 0x00;
  uint8_t *mem_;
};
