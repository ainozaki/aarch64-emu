#include "arm_decoder.h"

#include <cassert>
#include <stdio.h>

#include "arm.h"
#include "arm_op.h"
#include "mem.h"
#include "system.h"
#include "utils.h"

namespace core {
namespace decode {

namespace {

static void unsupported() { fprintf(stderr, "unsuported inst\n"); }

static void unallocated() { fprintf(stderr, "unallocated inst\n"); }

static inline uint64_t bitfield_replicate(uint64_t mask, uint8_t e) {
  while (e < 64) {
    mask |= mask << e;
    e *= 2;
  }
  return mask;
}

static inline uint64_t bitmask64(uint8_t length) {
  return ~0ULL >> (64 - length);
}

enum class ExtendType {
  UXTB,
  UXTH,
  UXTW,
  UXTX,
  SXTB,
  SXTH,
  SXTW,
  SXTX,
};

const ExtendType extendtype_tbl[] = {
    ExtendType::UXTB, ExtendType::UXTH, ExtendType::UXTW, ExtendType::UXTX,
    ExtendType::SXTB, ExtendType::SXTH, ExtendType::SXTW, ExtendType::SXTX,
};

// signed/unsigned extend
static inline uint64_t extend(uint64_t val, uint8_t len, bool if_unsigned) {
  if (if_unsigned) {
    return val & (1 << (len - 1));
  } else {
    return bitutil::bit(val, len) ? (val | 0xffffffffffffffff << len) : val;
  }
}

// ExtendReg() in ARM
// Perform a value extension and shift
uint64_t shift_and_extend(uint64_t val, uint64_t shift, ExtendType exttype) {
  bool if_unsigned;
  uint8_t len;

  assert(shift >= 0 && shift <= 4);
  if (shift) {
    val = val << shift;
  }
  switch (exttype) {
  case ExtendType::UXTB:
    if_unsigned = true;
    len = 8;
    break;
  case ExtendType::UXTH:
    if_unsigned = true;
    len = 16;
    break;
  case ExtendType::UXTW:
    if_unsigned = true;
    len = 32;
    break;
  case ExtendType::UXTX:
    if_unsigned = true;
    len = 64;
    break;
  case ExtendType::SXTB:
    if_unsigned = false;
    len = 8;
    break;
  case ExtendType::SXTH:
    if_unsigned = false;
    len = 16;
    break;
  case ExtendType::SXTW:
    if_unsigned = false;
    len = 32;
    break;
  case ExtendType::SXTX:
    if_unsigned = false;
    len = 64;
    break;
  }
  return extend(val, len, if_unsigned);
}

} // namespace

Decoder::Decoder(System *system) : system_(system) {}

void Decoder::start(uint32_t inst) {
  uint8_t op1;
  op1 = bitutil::shift(inst, 25, 28);
  const decode_func decode_inst_tbl[] = {
      &Decoder::decode_sme_encodings,
      &Decoder::decode_unallocated,
      &Decoder::decode_sve_encodings,
      &Decoder::decode_unallocated,
      &Decoder::decode_loads_and_stores,
      &Decoder::decode_data_processing_reg,
      &Decoder::decode_loads_and_stores,
      &Decoder::decode_data_processing_float,
      &Decoder::decode_data_processing_imm,
      &Decoder::decode_data_processing_imm,
      &Decoder::decode_branches,
      &Decoder::decode_branches,
      &Decoder::decode_loads_and_stores,
      &Decoder::decode_data_processing_reg,
      &Decoder::decode_loads_and_stores,
      &Decoder::decode_data_processing_float,
  };
  (this->*decode_inst_tbl[op1])(inst);
}

void Decoder::decode_data_processing_imm(uint32_t inst) {
  uint8_t op0;
  op0 = bitutil::shift(inst, 23, 25);
  const decode_func decode_data_processing_imm_tbl[] = {
      &Decoder::decode_pc_rel,      &Decoder::decode_pc_rel,
      &Decoder::decode_add_sub_imm, &Decoder::decode_add_sub_imm_with_tags,
      &Decoder::decode_logical_imm, &Decoder::decode_move_wide_imm,
      &Decoder::decode_bitfield,    &Decoder::decode_extract,
  };
  (this->*decode_data_processing_imm_tbl[op0])(inst);
}

void Decoder::decode_loads_and_stores(uint32_t inst) {
  uint8_t op;
  op = bitutil::shift(inst, 28, 29);
  switch (op) {
  case 0b00:
    unsupported();
    break;
  case 0b01:
    unsupported();
    break;
  case 0b10:
    unsupported();
    break;
  case 0b11:
    Decoder::decode_ldst_register(inst);
    break;
  }
}

void Decoder::decode_data_processing_reg(uint32_t inst){};
void Decoder::decode_data_processing_float(uint32_t inst){};
void Decoder::decode_branches(uint32_t inst){};
void Decoder::decode_pc_rel(uint32_t inst){};
void Decoder::decode_sme_encodings(uint32_t inst){};
void Decoder::decode_unallocated(uint32_t inst){};
void Decoder::decode_sve_encodings(uint32_t inst){};

/*
====================================================
         Data Processing Immediate
====================================================
*/

void Decoder::decode_add_sub_imm(uint32_t inst) {
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
      result = add_imm_s(system_->cpu().xregs[rn], ~imm, /*carry-in=*/1,
                         system_->cpu().cpsr);
    } else {
      /* ADDS */
      result = add_imm_s(system_->cpu().xregs[rn], imm, /*carry-in=*/0,
                         system_->cpu().cpsr);
    }
  } else {
    if (if_sub) {
      /* SUB */
      result = add_imm(system_->cpu().xregs[rn], ~imm, /*carry-in=*/1);
    } else {
      /* ADD */
      result = add_imm(system_->cpu().xregs[rn], imm, /*carry-in=*/0);
    }
  }

  if (if_64bit) {
    system_->cpu().xregs[rd] = result;
  } else {
    system_->cpu().xregs[rd] = bitutil::clear_upper32(result);
  }
}

void Decoder::decode_add_sub_imm_with_tags(uint32_t inst){};

static uint64_t decode_bit_masks(uint8_t n, uint8_t imms, uint8_t immr) {
  uint64_t mask;
  uint8_t e, levels, s, r;
  int len;

  assert(n < 2 && imms < 64 && immr < 64);

  len = 31 - __builtin_clz((n << 6) | (~imms & 0x3f));
  e = 1 << len;

  levels = e - 1;
  s = imms & levels;
  r = immr & levels;

  mask = bitmask64(s + 1);
  if (r) {
    mask = (mask >> r) | (mask << (e - r));
    mask &= bitmask64(e);
  }

  mask = bitfield_replicate(mask, e);
  return mask;
}

void Decoder::decode_logical_imm(uint32_t inst) {
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

  imm = decode_bit_masks(N, imms, immr);

  if (!if_64bit && N != 0) {
    fprintf(stderr, "undefined\n");
  }

  switch (opc) {
  case 0b00:
    result = system_->cpu().xregs[rn] & imm; /* AND */
    break;
  case 0b01:
    result = system_->cpu().xregs[rn] | imm; /* ORR */
    break;
  case 0b10:
    result = system_->cpu().xregs[rn] ^ imm; /* EOR */
    break;
  case 0b11:
    result = system_->cpu().xregs[rn] & imm; /* ANDS */
    system_->cpu().cpsr.N = result < 0;
    system_->cpu().cpsr.Z = result == 0;
    system_->cpu().cpsr.C = 0;
    system_->cpu().cpsr.V = 0;
    break;
  default:
    assert(false);
  }

  if (if_64bit) {
    system_->cpu().xregs[rd] = result;
  } else {
    system_->cpu().xregs[rd] = bitutil::clear_upper32(result);
  }
}

void Decoder::decode_move_wide_imm(uint32_t inst){};
void Decoder::decode_bitfield(uint32_t inst){};
void Decoder::decode_extract(uint32_t inst){};

/*
====================================================
         Loads and stores
====================================================
*/

void Decoder::decode_ldst_register(uint32_t inst) {
  uint8_t op;
  op = (bitutil::bit(inst, 21)) << 2 | bitutil::shift(inst, 10, 11);
  const decode_func decode_ldst_reg_tbl[] = {
      &Decoder::decode_ldst_reg_unscaled_imm,
      &Decoder::decode_ldst_reg_imm_post_indexed,
      &Decoder::decode_ldst_reg_unpriviledged,
      &Decoder::decode_ldst_reg_imm_pre_indexed,
      &Decoder::decode_ldst_atomic_memory_op,
      &Decoder::decode_ldst_reg_pac,
      &Decoder::decode_ldst_reg_reg_offset,
      &Decoder::decode_ldst_reg_pac,
  };
  (this->*decode_ldst_reg_tbl[op])(inst);
}

void Decoder::decode_ldst_reg_unscaled_imm(uint32_t inst) {
  printf("ldst_reg_unscaled_imm\n");
}
void Decoder::decode_ldst_reg_imm_post_indexed(uint32_t inst) {
  printf("ldst_reg_imm_post_indexed\n");
}
void Decoder::decode_ldst_reg_unpriviledged(uint32_t inst) {
  printf("ldst_reg_unpriviledged\n");
}
void Decoder::decode_ldst_reg_imm_pre_indexed(uint32_t inst) {
  printf("ldst_reg_imm_pre_indexed\n");
}
void Decoder::decode_ldst_atomic_memory_op(uint32_t inst) {
  printf("ldst_reg_atomic_memory_op\n");
}
void Decoder::decode_ldst_reg_pac(uint32_t inst) { printf("ldst_reg_pac\n"); }

/*
         Load/store register (reister offset)

         31   30 29 27 26  25 24 23 22 21  20   16 15 13 12  11 10 9  5 4   0
         +------+-----+---+----+------+---+-------+-----+---+-----+----+----+
         | size | 111 | V | 00 | opc  | 1 |   Rm  | opt | S | 10  | Rn | Rt |
         +------+-----+---+----+------+---+-------+-----+---+-----+----+----+

         @size:
                        00:  8bit
                        01: 16bit
                        10: 32bit
                        11: 64bit
         @V: simd
         @Rm: offset register
         @opt: extend type
         @S: if S=1 then shift |size|
         @Rn: base register or stack pointer
         @Rt: register to be transfered

*/
void Decoder::decode_ldst_reg_reg_offset(uint32_t inst) {
  bool vector, shift;
  uint8_t size, scale, opc, opt, rm, rn, rt;
  uint64_t offset;

  size = bitutil::shift(inst, 30, 31);
  scale = size;
  vector = bitutil::bit(inst, 26);
  opc = bitutil::shift(inst, 22, 23);
  rm = bitutil::shift(inst, 16, 20);
  opt = bitutil::shift(inst, 13, 15);
  shift = bitutil::bit(inst, 12);
  rn = bitutil::shift(inst, 5, 9);
  rt = bitutil::shift(inst, 0, 4);

  if (vector) {
    unsupported();
  } else {
    switch (opc) {
    case 0b00:
      /* store */
      // TODO: option = 0b011
      offset = shift_and_extend(system_->cpu().xregs[rm], scale,
                                extendtype_tbl[opt]);
      system_->mem().write(size, system_->cpu().xregs[rn] + offset,
                           system_->cpu().xregs[rt]);
      break;
    case 0b11: /* loads */
      printf("load ?\n");
      if (size >= 0b10) {
        unallocated();
      }
    case 0b01: /* loadu */
    case 0b10: /* loads */
               // TODO: option = 0b011
      offset = shift_and_extend(system_->cpu().xregs[rm], scale,
                                extendtype_tbl[opt]);
      system_->cpu().xregs[rt] =
          system_->mem().read(size, system_->cpu().xregs[rn] + offset);
      break;
    }
  }
}

} // namespace decode
} // namespace core
