#include "arm-op.h"

#include <cassert>
#include <cstdint>
#include <iostream>

#include <stdio.h>

#include "arm.h"
#include "utils.h"

static inline void set_Nflag(CPSR &cpsr, int64_t val) {
  cpsr.N = bitutil::bit(val, 63);
}

static inline void set_Zflag(CPSR &cpsr, int64_t val) { cpsr.Z = val == 0; }

static int64_t add_imm(int64_t x, int64_t y, int8_t carry_in) {
  return x + y + carry_in;
}

static int64_t add_imm_s(int64_t x, int64_t y, int8_t carry_in, CPSR &cpsr) {
  int64_t result = x + y + carry_in;

  set_Nflag(cpsr, result);
  set_Zflag(cpsr, result);
  // cpsr.C
  // cpsr.V
  return result;
}

static int64_t and_imm_s(int64_t x, int64_t y, CPSR &cpsr) {
  int64_t result = x & y;
  set_Nflag(cpsr, result);
  set_Zflag(cpsr, result);
  return result;
}

void disassemble_pc_rel(uint32_t inst){};

void disassemble_add_sub_imm(uint32_t inst, Cpu *cpu) {
  uint8_t rd, rn;
  uint16_t imm;
  bool shift, setflag, if_sub, if_64bit;
  uint64_t result;

  rd = bitutil::shift(inst, 0, 4);
  rn = bitutil::shift(inst, 5, 9);
  imm = bitutil::shift(inst, 10, 21);
  shift = bitutil::bit(inst, 22);
  setflag = bitutil::bit(inst, 29);
  if_sub = bitutil::bit(inst, 30);
  if_64bit = bitutil::bit(inst, 31);

  if (shift) {
    imm <<= 12;
  }

  if (setflag) {
    if (if_sub) {
      /* SUBS */
      result = add_imm_s(cpu->xregs[rn], ~imm, /*carry-in=*/1, cpu->cpsr);
    } else {
      /* ADDS */
      result = add_imm_s(cpu->xregs[rn], imm, /*carry-in=*/0, cpu->cpsr);
    }
  } else {
    if (if_sub) {
      /* SUB */
      result = add_imm(cpu->xregs[rn], ~imm, /*carry-in=*/1);
    } else {
      /* ADD */
      result = add_imm(cpu->xregs[rn], imm, /*carry-in=*/0);
    }
  }

  if (if_64bit) {
    cpu->xregs[rd] = result;
  } else {
    cpu->xregs[rd] = bitutil::clear_upper32(result);
  }
}

void disassemble_add_sub_imm_with_tags(uint32_t inst){};

void disassemble_logical_imm(uint32_t inst, Cpu *cpu) {
  uint8_t rd, rn, imms, immr, opc;
  bool N, if_64bit;
  uint32_t imm;
  uint64_t result;

  rd = bitutil::shift(inst, 0, 4);
  rn = bitutil::shift(inst, 5, 9);
  imms = bitutil::shift(inst, 10, 15);
  immr = bitutil::shift(inst, 16, 21);
  N = bitutil::bit(inst, 22);
  opc = bitutil::shift(inst, 29, 30);
  if_64bit = bitutil::bit(inst, 31);

  imm = imms; // TODO: implement DecodeBitMasks

  if (!if_64bit && N != 0) {
    fprintf(stderr, "undefined\n");
  }

  switch (opc) {
  case 0b00:
  case 0b11:
    result = cpu->xregs[rn] & imm; /* AND */
    break;
  case 0b01:
    result = cpu->xregs[rn] | imm; /* ORR */
    break;
  case 0b10:
    result = cpu->xregs[rn] ^ imm; /* EOR */
    break;
  default:
    assert(false);
  }

  if (if_64bit) {
    cpu->xregs[rd] = result;
  } else {
    cpu->xregs[rd] = bitutil::clear_upper32(result);
  }
}

void disassemble_move_wide_imm(uint32_t inst){};
void disassemble_bitfield(uint32_t inst){};
void disassemble_extract(uint32_t inst){};
