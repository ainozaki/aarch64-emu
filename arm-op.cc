#include "arm-op.h"

#include <cassert>
#include <cstdint>
#include <iostream>

#include <stdio.h>

#include "arm.h"
#include "utils.h"

void set_Nflag(CPSR &cpsr, uint64_t val) { cpsr.N = bitutil::bit(val, 63); }

void set_Zflag(CPSR &cpsr, uint64_t val) { cpsr.Z = val == 0; }

uint64_t add_imm(uint64_t x, uint64_t y, uint8_t carry_in) {
  return x + y + carry_in;
}

uint64_t add_imm_s(uint64_t x, uint64_t y, uint8_t carry_in, CPSR &cpsr) {
  int64_t result = x + y + carry_in;

  set_Nflag(cpsr, result);
  set_Zflag(cpsr, result);
  // cpsr.C
  // cpsr.V
  return result;
}
