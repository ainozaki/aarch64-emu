#include "arm-op.h"

#include <cstdint>
#include <iostream>

#include "arm.h"

uint64_t add_imm(uint64_t x, uint64_t y, uint8_t carry_in) {
  return x + y + carry_in;
}

uint64_t add_imm_s(uint64_t x, uint64_t y, uint8_t carry_in,
                   struct CPSR &cpsr) {
  uint64_t result;
  result = x + y + carry_in;
  cpsr.N = result >= 0 ? 1 : 0;
  cpsr.Z = !result ? 1 : 0;
  // cpsr.C
  // cpsr.V
  std::cout << "CPSR: " << cpsr.N << cpsr.Z << std::endl;
  return x + y;
}
