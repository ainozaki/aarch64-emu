#include "arm-op.h"

#include <cstdint>
#include <iostream>

#include <stdio.h>

#include "arm.h"
#include "utils.h"

static inline void set_Nflag(CPSR &cpsr, int64_t val) {
  cpsr.N = bitutil::bit(val, 63);
}

static inline void set_Zflag(CPSR &cpsr, int64_t val) { cpsr.Z = val == 0; }

int64_t add_imm(int64_t x, int64_t y, int8_t carry_in) {
  return x + y + carry_in;
}

int64_t add_imm_s(int64_t x, int64_t y, int8_t carry_in, CPSR &cpsr) {
  int64_t result = x + y + carry_in;

  set_Nflag(cpsr, result);
  set_Zflag(cpsr, result);
  // cpsr.C
  // cpsr.V

  return result;
}
