#include "utils.h"

#include <cstdint>
#include <stdio.h>

#include "mem.h"

namespace util {
uint64_t shift_with_type(uint64_t value, uint8_t type, uint8_t amount) {
  uint8_t x;
  switch (type) {
  case 0: // LSL
    return value << amount;
  case 1: // LSR
    return value >> amount;
  case 2: // ASR
    return (int64_t)value >> amount;
  case 3: // ROR (Rotate Right)
    x = 64 - amount;
    return ((value >> amount) & mask(x)) | ((value >> x) << x);
  default:
    fprintf(stderr, "Unknown shift type\n");
    return 0;
  }
}

uint64_t set_lower(uint64_t dst, uint64_t src, MemAccessSize size) {
  switch (size) {
  case MemAccessSize::Byte:
    return (dst & ~mask(8)) | (src & mask(8));
  case MemAccessSize::Hex:
    return (dst & ~mask(16)) | (src & mask(16));
  case MemAccessSize::Word:
    return (dst & ~mask(32)) | (src & mask(32));
  case MemAccessSize::DWord:
    return src;
  default:
    assert(false);
    // dummy
    return 0;
  }
}

uint64_t set_lower32(uint64_t dst, uint64_t src) {
  return (dst & ~mask(32)) | (src & mask(32));
}
} // namespace util