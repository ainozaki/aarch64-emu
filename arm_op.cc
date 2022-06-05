#include "arm_op.h"

#include <cassert>
#include <cstdint>
#include <iostream>

#include <stdio.h>

#include "arm.h"
#include "utils.h"

namespace core {

void set_Nflag(core::cpu::CPSR &cpsr, uint64_t val) {
  cpsr.N = bitutil::bit(val, 63);
}

void set_Zflag(core::cpu::CPSR &cpsr, uint64_t val) { cpsr.Z = val == 0; }

uint64_t add_imm(uint64_t x, uint64_t y, uint8_t carry_in) {
  return x + y + carry_in;
}

uint64_t add_imm_s(uint64_t x, uint64_t y, uint8_t carry_in,
                   core::cpu::CPSR &cpsr) {
  uint64_t unsigned_sum = x + y + carry_in;
  int64_t signed_sum = (int64_t)x + (int64_t)y + carry_in;

  cpsr.N = signed_sum < 0;
  cpsr.Z = unsigned_sum == 0;
  cpsr.C = unsigned_sum < x;
  cpsr.V = ((int64_t)x < 0 && (int64_t)y < 0 && signed_sum > 0) |
           ((int64_t)x > 0 && (int64_t)y > 0 && signed_sum < 0);
  return unsigned_sum;
}

} // namespace core
