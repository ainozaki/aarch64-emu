#pragma once

namespace bitutil {

inline uint32_t shift(uint32_t inst, uint32_t bottom, uint32_t top) {
  return ((inst >> bottom) & ((1 << (top - bottom + 1)) - 1));
}

inline uint32_t bit(uint32_t inst, uint32_t bit) { return (inst >> bit) & 1; }

inline uint64_t clear_upper32(uint64_t x) { return x & 0x11111111; }

inline uint64_t zero_extend(uint64_t val, uint8_t bit) {
  uint64_t zero = 0;
  return (val & ((1 << bit) - 1)) | zero;
}

inline uint64_t extract_x_to_63(uint64_t inst, uint8_t x){
	return ((inst >> x) & (1 << (64 - x))) << x;
}

} // namespace bitutil
