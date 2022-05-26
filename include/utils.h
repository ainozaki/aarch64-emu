#pragma once

namespace bitutil {
inline uint32_t shift(uint32_t inst, uint32_t bottom, uint32_t top) {
  return ((inst >> bottom) & ((1 << (top - bottom + 1)) - 1));
}

inline uint32_t bit(uint32_t inst, uint32_t bit) { return (inst >> bit) & 1; }
} // namespace bitutil
