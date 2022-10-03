#include "utils.h"

#include <cstdint>
#include <stdio.h>
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

uint64_t set_lower32(uint64_t original, uint64_t setvalue) {
  return (original & ~mask(32)) | (setvalue & mask(32));
}
} // namespace util