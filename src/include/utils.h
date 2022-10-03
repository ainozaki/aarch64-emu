#pragma once

#include <cassert>
#include <cstdint>

namespace util {

inline uint64_t LSL(uint64_t val, uint8_t shift) {
  return shift >= 64 ? 0 : val << shift;
}

inline uint64_t LSR(uint64_t val, uint8_t shift) {
  return shift >= 64 ? 0 : val >> shift;
}

inline uint64_t ASR(uint64_t val, uint8_t shift) {
  return shift >= 64 ? 0 : (int64_t)val >> shift;
}

inline uint64_t ROR(uint64_t val, uint8_t shift) {
  return (val << (64 - shift)) | (val >> shift);
}

inline uint64_t SIGN_EXTEND(uint64_t val, uint8_t topbit) {
  return ASR(LSL(val, 64 - (topbit)), 64 - (topbit));
}

inline uint32_t shift(uint32_t inst, uint32_t bottom, uint32_t top) {
  return ((inst >> bottom) & ((1 << (top - bottom + 1)) - 1));
}

inline uint32_t bit(uint32_t inst, uint8_t bit) { return (inst >> bit) & 1; }
inline uint64_t bit64(uint64_t inst, uint8_t bit) { return (inst >> bit) & 1; }

inline uint64_t clear_upper32(uint64_t x) { return x & 0xffffffff; }

inline uint64_t zero_extend(uint64_t val, uint8_t bit) {
  uint64_t zero = 0;
  return (val & ((1 << bit) - 1)) | zero;
}

inline uint64_t mask(uint8_t n) {
  assert(0 <= n <= 64);
  if (n == 64) {
    return ~uint64_t(0);
  }
  return (uint64_t(1) << n) - 1;
}

inline uint64_t extract(uint64_t value, uint8_t start, uint8_t len) {
  return value & (mask(start + 1) & ((uint64_t)1 << (start - len + 1)));
}

inline uint64_t extract_upper32(uint64_t value) { return value & ~mask(32); }

uint64_t set_lower32(uint64_t original, uint64_t setvalue);

uint64_t shift_with_type(uint64_t value, uint8_t type, uint8_t amount);
} // namespace util
