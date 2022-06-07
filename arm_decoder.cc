#include "arm_decoder.h"

#include <cassert>
#include <stdio.h>

#include "arm.h"
#include "arm_op.h"
#include "log.h"
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

const int size_tbl[] = {8, 16, 32, 64};

const ExtendType extendtype_tbl[] = {
    ExtendType::UXTB, ExtendType::UXTH, ExtendType::UXTW, ExtendType::UXTX,
    ExtendType::SXTB, ExtendType::SXTH, ExtendType::SXTW, ExtendType::SXTX,
};

// signed extend
static inline uint64_t signed_extend(uint64_t val, uint8_t topbit) {
  return bitutil::bit(val, topbit) ? (val | 0xffffffffffffffff << topbit) : val;
}

// ExtendReg() in ARM
// Perform a value extension and shift
uint64_t shift_and_extend(uint64_t val, bool shift, uint8_t scale,
                          ExtendType exttype) {
  bool if_unsigned;
  uint8_t len;

  if (shift) {
    assert(scale >= 0 && scale <= 4);
    val = val << scale;
  }
  switch (exttype) {
  case ExtendType::UXTB:
  case ExtendType::UXTH:
  case ExtendType::UXTW:
  case ExtendType::UXTX:
    if_unsigned = true;
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
  // printf("shift_and_extend: val=%lu, shift=%d, scale=%d, len=%d\n", val,
  // shift, scale, len);
  if (if_unsigned) {
    return val;
  } else {
    return signed_extend(val, len);
  }
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

  if (bitutil::bit(inst, 24)) {
    decode_ldst_reg_unsigned_imm(inst);
  } else {
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
}

void Decoder::decode_ldst_reg_unsigned_imm(uint32_t inst) {
  LOG_CPU("load_store: reg_unsigned_imm\n");
}

/*
         Load/store register (unscaled immediate)

         31   30 29 27 26  25 24 23 22 21  20      12 11 10  9  5 4   0
         +------+-----+---+----+------+---+----------+-----+-----+-----+
         | size | 111 | V | 00 | opc  | 0 |   imm9   | 00  |  Rn |  Rt |
         +------+-----+---+----+------+---+----------+-----+-----+-----+

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

void Decoder::decode_ldst_reg_unscaled_imm(uint32_t inst) {
  bool vector;
  uint8_t size, opc, rn, rt;
  uint64_t imm9, offset;

  size = bitutil::shift(inst, 30, 31);
  vector = bitutil::bit(inst, 26);
  opc = bitutil::shift(inst, 22, 23);
  imm9 = bitutil::shift(inst, 12, 20);
  rn = bitutil::shift(inst, 5, 9);
  rt = bitutil::shift(inst, 0, 4);

  if (vector) {
    unsupported();
  } else {
    offset = signed_extend(imm9, 9);
    switch (opc) {
    case 0b00:
      /* store */
      /* STURB, STURH, STUR(32-bit, 64-bit) */
      LOG_CPU("load_store: register: unscaled imm: store: rt=%d, rn=%d, "
              "offset=%lu, size=%d\n",
              rt, rn, offset, size_tbl[size]);
      system_->mem().write(size, system_->cpu().xregs[rn] + offset,
                           system_->cpu().xregs[rt]);
      break;
    case 0b01:
      /* loadu */
      /* LDURB, LDURH, LDUR(32-bit, 64-bit) */
      // TODO: load only lower-32bit when size != 0b11
      LOG_CPU("LDUR{B,H,}: rt=%d, rn=%d, offset=%lu, size=%d\n", rt, rn, offset,
              size_tbl[size]);
      system_->cpu().xregs[rt] =
          system_->mem().read(size, system_->cpu().xregs[rn] + offset);
      break;
    case 0b10:
      /* LDURSB, LDURSH, LDURSW(64bit variant), PRFUM */
      if (size == 3) {
        unsupported(); // PRFUM
        break;
      }
      LOG_CPU("LDURS{B,H,W}: rt=%d, rn=%d, offset=%lu, size=%d\n", rt, rn,
              offset, size_tbl[size]);
      system_->cpu().xregs[rt] = signed_extend(
          system_->mem().read(size, system_->cpu().xregs[rn] + offset),
          size_tbl[size]);
      break;
    case 0b11:
      if (size >= 2) {
        unallocated();
      }
      /* LDURSB, LDURSH (32bit variant) */
      if (size == 3) {
        // TODO: load only lower-32bit when size
        LOG_CPU("LDURS{B,H} rt=%d, rn=%d, offset=%lu, size=%d\n", rt, rn,
                offset, size_tbl[size]);
        system_->cpu().xregs[rt] = signed_extend(
            system_->mem().read(size, system_->cpu().xregs[rn] + offset),
            size_tbl[size]);
        break;
      }
    }
  }
}

void Decoder::decode_ldst_reg_imm_post_indexed(uint32_t inst) {
  LOG_CPU("load_store: ldst_reg_imm_post_indexed\n");
}
void Decoder::decode_ldst_reg_unpriviledged(uint32_t inst) {
  LOG_CPU("load_store: ldst_reg_unpriviledged\n");
}
void Decoder::decode_ldst_reg_imm_pre_indexed(uint32_t inst) {
  LOG_CPU("load_store: ldst_reg_imm_pre_indexed\n");
}
void Decoder::decode_ldst_atomic_memory_op(uint32_t inst) {
  LOG_CPU("load_store: ldst_reg_atomic_memory_op\n");
}
void Decoder::decode_ldst_reg_pac(uint32_t inst) {
  LOG_CPU("load_store: ldst_reg_pca\n");
}

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
      offset = shift_and_extend(system_->cpu().xregs[rm], shift, scale,
                                extendtype_tbl[opt]);
      system_->mem().write(size, system_->cpu().xregs[rn] + offset,
                           system_->cpu().xregs[rt]);
      LOG_CPU("load_store: register offset: store: rt %d [rn] %lu offset %lu\n",
              rt, system_->cpu().xregs[rn], offset);
      break;
    case 0b11: /* loads */
      if (size >= 0b10) {
        unallocated();
      }
    case 0b01: /* loadu */
    case 0b10: /* loads */
               // TODO: option = 0b011
      offset = shift_and_extend(system_->cpu().xregs[rm], shift, scale,
                                extendtype_tbl[opt]);
      LOG_CPU(
          "load_store: register offset: load: rt=%d, rn=%d, [rn]=0x%lx, rm=%d, "
          "[rm]=0x%lx, offset=%lu vaddr=0x%lx\n",
          rt, rn, system_->cpu().xregs[rn], rm, system_->cpu().xregs[rm],
          offset, system_->cpu().xregs[rn] + offset);
      system_->cpu().xregs[rt] =
          system_->mem().read(size, system_->cpu().xregs[rn] + offset);
      break;
    }
  }
}

} // namespace decode
} // namespace core
