#pragma once

#include <cassert>
#include <cstdint>
#include <unistd.h>
#include "bus.h"

//#define DEBUG_ON
#ifdef DEBUG_ON
#define LOG_EMU(...)                                          \
  printf("[%s][%d][%s] ", __FILE__, __LINE__, __func__), \
      printf(__VA_ARGS__)
#else
#define LOG_EMU(...)
#endif

const uint64_t PAGE_SIZE = sysconf(_SC_PAGESIZE);

namespace util {

inline uint64_t PAGE_ROUNDUP(uint64_t v) {
  return (v + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

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

// mask n digits
// ex) mask(3) returns 0b111
inline uint64_t mask(uint8_t n) {
  assert(0 <= n <= 64);
  if (n == 64) {
    return ~uint64_t(0);
  }
  return (uint64_t(1) << n) - 1;
}

inline uint64_t shift64(uint64_t inst, uint32_t bottom, uint32_t top) {
  return (inst & mask(top + 1)) >> bottom;
}

inline uint32_t shift(uint32_t inst, uint32_t bottom, uint32_t top) {
  if ((bottom >= 32) || (top >= 32)) {
    return shift64(inst, bottom, top);
  }
  return (inst & mask(top + 1)) >> bottom;
}

inline uint32_t bit(uint32_t inst, uint8_t bit) { return (inst >> bit) & 1; }
inline uint64_t bit64(uint64_t inst, uint8_t bit) { return (inst >> bit) & 1; }

inline uint64_t clear_upper32(uint64_t x) { return x & 0xffffffff; }

inline uint64_t zero_extend(uint64_t val, uint8_t bit) {
  uint64_t zero = 0;
  return (val & ((1 << bit) - 1)) | zero;
}

inline uint64_t extract(uint64_t value, uint8_t start, uint8_t len) {
  return value & (mask(start + 1) & ((uint64_t)1 << (start - len + 1)));
}

inline uint64_t extract_upper32(uint64_t value) { return value & ~mask(32); }

uint64_t set_lower32(uint64_t dst, uint64_t src);
uint64_t set_lower(uint64_t dst, uint64_t src, MemAccessSize size);

uint64_t shift_with_type(uint64_t value, uint8_t type, uint8_t amount);
} // namespace util
