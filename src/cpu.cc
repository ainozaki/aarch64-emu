#include "cpu.h"

#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <unistd.h>

#include "log.h"
#include "utils.h"

typedef __attribute__((mode(TI))) unsigned int uint128_t;
typedef __attribute__((mode(TI))) int int128_t;

uint32_t Cpu::fetch() { return bus.load32(pc); }

void Cpu::decode_start(uint32_t inst) {
  uint8_t op1;
  op1 = util::shift(inst, 25, 28);

  // LOG_CPU("PC=0x%lx\n", pc);
  const decode_func decode_inst_tbl[] = {
      &Cpu::decode_sme_encodings,
      &Cpu::decode_unallocated,
      &Cpu::decode_sve_encodings,
      &Cpu::decode_unallocated,
      &Cpu::decode_loads_and_stores,
      &Cpu::decode_data_processing_reg,
      &Cpu::decode_loads_and_stores,
      &Cpu::decode_data_processing_float,
      &Cpu::decode_data_processing_imm,
      &Cpu::decode_data_processing_imm,
      &Cpu::decode_branches,
      &Cpu::decode_branches,
      &Cpu::decode_loads_and_stores,
      &Cpu::decode_data_processing_reg,
      &Cpu::decode_loads_and_stores,
      &Cpu::decode_data_processing_float,
  };
  (this->*decode_inst_tbl[op1])(inst);
}

namespace {
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

const ExtendType extendtype_tbl[8] = {
    ExtendType::UXTB, ExtendType::UXTH, ExtendType::UXTW, ExtendType::UXTX,
    ExtendType::SXTB, ExtendType::SXTH, ExtendType::SXTW, ExtendType::SXTX,
};

uint64_t ExtendValue(uint64_t value, uint8_t type, uint8_t shift) {
  ExtendType etype = extendtype_tbl[type];
  bool if_unsigned;
  uint8_t len;
  assert((shift >= 0) && (shift <= 4));

  switch (etype) {
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
    len = 8;
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
  len = std::min<uint8_t>(len, 64 - shift);
  value = (value & util::mask(len)) << shift;
  return if_unsigned ? value : util::SIGN_EXTEND(value, len + shift);
}

const int size_tbl[] = {8, 16, 32, 64};

const char *shift_type_strtbl[] = {
    "LSL",
    "LSR",
    "ASR",
    "ROR",
};

static void unsupported() { LOG_CPU("unsuported inst\n"); }

static void unallocated() { LOG_CPU("unallocated inst\n"); }

static inline uint64_t bitmask64(uint8_t len) { return ~0ULL >> (64 - len); }

static inline uint64_t signed_extend(uint64_t val, uint8_t topbit) {
  return util::bit(val, topbit) ? (val | ~util::mask(topbit)) : val;
}

static inline uint32_t signed_extend32(uint32_t val, uint8_t topbit) {
  return util::bit(val, topbit) ? (val | ~uint32_t(0) << topbit) : val;
}

} // namespace

void Cpu::show_regs() {
  std::cout << "=================================================" << std::endl;
  printf("registers:\n");
  for (int i = 0; i < 32; i += 4) {
    printf("\tw%2d: 0x%16lx", i, xregs[i]);
    printf("\tw%2d: 0x%16lx", i + 1, xregs[i + 1]);
    printf("\tw%2d: 0x%16lx", i + 2, xregs[i + 2]);
    printf("\tw%2d: 0x%16lx\n", i + 3, xregs[i + 3]);
  }
  std::cout << std::endl;
  std::cout << "CPSR: [NZCV]=" << cpsr.N << cpsr.Z << cpsr.C << cpsr.Z
            << std::endl;
  std::cout << "=================================================" << std::endl;
}

void Cpu::update_lower32(uint8_t reg, uint32_t value) {
  xregs[reg] = (xregs[reg] & (uint64_t)0xffffffff << 32) | value;
}

void Cpu::decode_data_processing_imm(uint32_t inst) {
  uint8_t op0;
  op0 = util::shift(inst, 23, 25);
  const decode_func decode_data_processing_imm_tbl[] = {
      &Cpu::decode_pc_rel,      &Cpu::decode_pc_rel,
      &Cpu::decode_add_sub_imm, &Cpu::decode_add_sub_imm_with_tags,
      &Cpu::decode_logical_imm, &Cpu::decode_move_wide_imm,
      &Cpu::decode_bitfield,    &Cpu::decode_extract,
  };
  (this->*decode_data_processing_imm_tbl[op0])(inst);
  increment_pc();
}

void Cpu::decode_loads_and_stores(uint32_t inst) {
  uint8_t op;
  op = util::shift(inst, 28, 29);
  switch (op) {
  case 0b00:
    unsupported();
    break;
  case 0b01:
    unsupported();
    break;
  case 0b10:
    Cpu::decode_ldst_register_pair(inst);
    break;
  case 0b11:
    Cpu::decode_ldst_register(inst);
    break;
  }
  increment_pc();
}

void Cpu::decode_data_processing_reg(uint32_t inst) {
  uint8_t op1, op2;

  op1 = util::bit(inst, 28);
  op2 = util::shift(inst, 21, 24);
  switch (op1) {
  case 0:
    if (op2 < 8) {
      decode_logical_shifted_reg(inst);
    } else if (op2 % 2 == 0) {
      decode_addsub_shifted_reg(inst);
    } else {
      decode_addsub_extended_reg(inst);
    }
    break;
  case 1:
    switch (op2) {
    case 0:
      break;
    case 2:
      printf("conditional branch\n");
      unsupported();
      break;
    case 4:
      decode_conditional_select(inst);
      break;
    case 6:
      printf("data processing 1 or 2 source\n");
      unsupported();
      break;
    case 1:
    case 3:
    case 5:
    case 7:
      unallocated();
      break;
    default:
      printf("data processing 3 source\n");
      unsupported();
      break;
    }
    break;
  default:
    assert(false);
  }
  increment_pc();
}

void Cpu::decode_data_processing_float(uint32_t inst) {
  LOG_CPU("data_processing_float %d\n", inst);
  increment_pc();
}

void Cpu::decode_branches(uint32_t inst) {
  uint8_t op;
  op = util::shift(inst, 29, 31);
  switch (op) {
  case 0:
  case 4:
    decode_unconditional_branch_imm(inst);
    break;
  case 2:
    decode_conditional_branch_imm(inst);
    break;
  case 1:
  case 5:
    if (util::bit(inst, 25) == 0) {
      decode_compare_and_branch_imm(inst);
    } else {
      unsupported();
    }
    break;
  case 6:
    switch (util::shift(inst, 24, 25)) {
    case 0:
      decode_exception_generation(inst);
      break;
    case 2:
    case 3:
      decode_unconditional_branch_reg(inst);
      break;
    default:
      unsupported();
    }
    break;
  default:
    unsupported();
    break;
  }
}

void Cpu::decode_sme_encodings(uint32_t inst) {
  LOG_CPU("sme_encodings %d\n", inst);
  increment_pc();
}

void Cpu::decode_unallocated(uint32_t inst) {
  LOG_CPU("unallocated %d\n", inst);
  increment_pc();
}

void Cpu::decode_sve_encodings(uint32_t inst) {
  LOG_CPU("sve_encodings %d\n", inst);
  increment_pc();
}

/*
====================================================
         Data Processing Immediate
====================================================
*/

/*
         PC-rel.addressing

           31   30  29 28    24 23           5 4   0
         +----+-------+-------+---------------+-----+
         | op | immlo | 10000 |    immhi      |  Rd |
         +----+-------+-------+---------------+-----+

         @op: 0->ADR, 1->ADRP
*/
void Cpu::decode_pc_rel(uint32_t inst) {
  uint8_t op, rd;
  uint64_t imm, immhi, immlo;

  op = util::bit(inst, 31);
  immlo = util::shift(inst, 29, 30);
  immhi = util::shift(inst, 5, 23);
  rd = util::shift(inst, 0, 4);

  switch (op) {
  case 0:
    imm = (immhi << 21) | immlo;
    xregs[rd] = (pc & ~util::mask(12)) + imm;
    LOG_CPU("adr x%d(=0x%lx), 0x%lx\n", rd, xregs[rd], imm);
    break;
  case 1:
    imm = immhi << 14;
    imm = imm | (immlo << 12);
    imm = util::SIGN_EXTEND(imm, 33);
    xregs[rd] = (pc & ~util::mask(12)) + imm;
    LOG_CPU("adrp x%d(=0x%lx), 0x%lx\n", rd, xregs[rd], imm);
    break;
  default:
    unallocated();
    return;
  }
}

static uint64_t add_imm(uint64_t x, uint64_t y, uint8_t carry_in) {
  if (carry_in) {
    return x + ~y + carry_in;
  }
  return x + y + carry_in;
}

static uint32_t add_imm_s32(uint32_t x, uint32_t y, uint8_t carry_in,
                            CPSR &cpsr) {
  if (carry_in) {
    y = ~y;
  }
  int32_t sx = (int32_t)x;
  int32_t sy = (int32_t)y;
  uint64_t unsigned_sum = (uint64_t)x + (uint64_t)y + (uint64_t)carry_in;
  int64_t signed_sum = (int64_t)sx + (int64_t)sy + (uint64_t)carry_in;
  uint32_t result = unsigned_sum & util::mask(32);

  cpsr.N = util::bit(result, 31);
  cpsr.Z = result == 0;
  cpsr.C = unsigned_sum != result;
  cpsr.V = signed_sum != (int32_t)result;
  return result;
}

static uint64_t add_imm_s(uint64_t x, uint64_t y, uint8_t carry_in, CPSR &cpsr,
                          bool if_64bit) {
  if (!if_64bit) {
    return add_imm_s32(x, y, carry_in, cpsr);
  }
  if (carry_in) {
    y = ~y;
  }
  int64_t sx = (int64_t)x;
  int64_t sy = (int64_t)y;
  uint128_t unsigned_sum = (uint128_t)x + (uint128_t)y + (uint128_t)carry_in;
  int128_t signed_sum = (int128_t)sx + (int128_t)sy + (uint128_t)carry_in;
  uint64_t result = unsigned_sum & util::mask(64);

  cpsr.N = util::bit64(result, 63);
  cpsr.Z = result == 0;
  cpsr.C = unsigned_sum != result;
  cpsr.V = signed_sum != (int64_t)result;
  return result;
}

/*
         Add/substract (immediate)

           31   30   29 28    23  22  21          10 9   5 4   0
         +----+----+---+--------+----+--------------+-----+-----+
         | sf | op | S | 100010 | sh |     imm12    |  Rn |  Rd |
         +----+----+---+--------+----+--------------+-----+-----+

         @sf: 0->32bit, 1->64bit
         @op: 0->ADD, 1->SUB
         @S: set flag
         @sh: if sh=1 then LSL #12
         @Rn: source gp regiter or sp
         @Rd: destination gp register or sp

*/
void Cpu::decode_add_sub_imm(uint32_t inst) {
  uint8_t rd, rn;
  uint64_t imm;
  bool if_shift, if_setflag, if_sub, if_64bit;
  uint64_t result, op1;
  const char *op;

  rd = util::shift(inst, 0, 4);
  rn = util::shift(inst, 5, 9);
  imm = util::shift(inst, 10, 21);
  if_shift = util::bit(inst, 22);
  if_setflag = util::bit(inst, 29);
  if_sub = util::bit(inst, 30);
  if_64bit = util::bit(inst, 31);
  op = if_sub ? "sub" : "add";

  op1 = xregs[rn];
  if (if_shift) {
    imm <<= 12;
  }

  if (if_setflag) {
    result = add_imm_s(op1, imm, if_sub, cpsr, if_64bit);
    LOG_CPU("%ss x%d, x%d, #0x%lx, LSL %d\n", op, rd, rn, imm, if_shift * 12);
  } else {
    result = add_imm(op1, imm, if_sub);
    LOG_CPU("%s x%d, x%d, #0x%lx, LSL %d\n", op, rd, rn, imm, if_shift * 12);
  }

  xregs[rd] = if_64bit ? result : util::set_lower32(xregs[rd], result);
}

void Cpu::decode_add_sub_imm_with_tags(uint32_t inst) { LOG_CPU("%d\n", inst); }

static uint64_t bitfield_replicate(uint64_t mask, uint8_t e) {
  while (e < 64) {
    mask |= mask << e;
    e *= 2;
  }
  return mask;
}

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

/*
         Logical (immediate)

           31   30 29 28    23 22  21   16 15  10 9  5 4   0
         +----+------+--------+---+-------+------+----+-----+
         | sf |  opc | 100100 | N |  immr | imms | Rn |  Rd |
         +----+------+--------+---+-------+------+----+-----+

         @sf: 0->32bit, 1->64bit
         @opc: 0->AND, 1->ORR, 2->EOR, 3->ANDS
*/

void Cpu::decode_logical_imm(uint32_t inst) {
  uint8_t rd, rn, imms, immr, opc;
  bool N, if_64bit;
  uint32_t imm;
  uint64_t result;

  rd = util::shift(inst, 0, 4);
  rn = util::shift(inst, 5, 9);
  imms = util::shift(inst, 10, 15);
  immr = util::shift(inst, 16, 21);
  N = util::bit(inst, 22);
  opc = util::shift(inst, 29, 30);
  if_64bit = util::bit(inst, 31);

  imm = decode_bit_masks(N, imms, immr);

  if (!if_64bit & (N == 1)) {
    unallocated();
    return;
  }

  switch (opc) {
  case 0b00:
    LOG_CPU("AND: [rn]=%lu, imm=%d\n", xregs[rn], imm);
    result = xregs[rn] & imm; /* AND */
    break;
  case 0b01:
    LOG_CPU("ORR: [rn]=%lu, imm=%d\n", xregs[rn], imm);
    result = xregs[rn] | imm; /* ORR */
    break;
  case 0b10:
    LOG_CPU("EOR: [rn]=%lu, imm=%d\n", xregs[rn], imm);
    result = xregs[rn] ^ imm; /* EOR */
    break;
  case 0b11:
    LOG_CPU("ANDS: [rn]=%lu, imm=%d\n", xregs[rn], imm);
    result = xregs[rn] & imm; /* ANDS */
    if (if_64bit) {
      cpsr.N = (int64_t)result < 0;
    } else {
      cpsr.N = (int32_t)result < 0;
    }
    cpsr.Z = result == 0;
    cpsr.C = 0;
    cpsr.V = 0;
    break;
  default:
    assert(false);
  }

  if (if_64bit) {
    xregs[rd] = result;
  } else {
    xregs[rd] = util::clear_upper32(result);
  }
}

/*
         Move wide (immediate)

           31   30 29 28    23 22 21 20           5 4   0
         +----+------+--------+-----+--------------+-----+
         | sf |  opc | 100101 |  hw |     imm16    |  Rd |
         +----+------+--------+-----+--------------+-----+

         @sf: 0->32bit, 1->64bit
         @opc: 00->N, 01->unallocated, 10->Z, 11->K
         @hw: 1->unallocated
         @Rd: destination gp register or sp

*/
void Cpu::decode_move_wide_imm(uint32_t inst) {
  uint8_t opc, hw, rd, shift;
  bool if_64bit;
  uint64_t imm;

  rd = util::shift(inst, 0, 4);
  imm = util::shift(inst, 5, 20);
  hw = util::shift(inst, 21, 22);
  opc = util::shift(inst, 29, 30);
  if_64bit = util::bit(inst, 31);

  if (opc == 1 || ((if_64bit == 0) & (hw >= 2))) {
    unallocated();
    return;
  }

  shift = hw * 16;
  imm = imm << shift;

  switch (opc) {
  case 0: /* MOVN */
    imm = ~imm;
    [[fallthrough]];
  case 2: /* MOVZ */
    xregs[rd] =
        if_64bit ? imm : (xregs[rd] & ~util::mask(32)) | (imm & util::mask(32));
    break;
  case 3: /* MOVK */
    xregs[rd] = ~util::extract(xregs[rd], 4 * shift + 16, 16) | imm;
    break;
  default:
    assert(false);
  }
  LOG_CPU("mov x%d, #0x%lx\n", rd, xregs[rd]);
}

/*
         Bitfield Move

           31   30 29 28    23 22  21   16 15  10 9  5 4   0
         +----+------+--------+---+-------+------+----+-----+
         | sf |  opc | 100110 | N |  immr | imms | Rn |  Rd |
         +----+------+--------+---+-------+------+----+-----+

         @sf: 0->32bit, 1->64bit
         @opc: 0->SBFM, 1->BFM, 2->UBFM, 3->unallocated

*/
void Cpu::decode_bitfield(uint32_t inst) {
  uint8_t rd, rn, imms, immr, opc, to, from, len;
  bool N, if_64bit;
  uint32_t imm;

  rd = util::shift(inst, 0, 4);
  rn = util::shift(inst, 5, 9);
  imms = util::shift(inst, 10, 15);
  immr = util::shift(inst, 16, 21);
  N = util::bit(inst, 22);
  opc = util::shift(inst, 29, 30);
  if_64bit = util::bit(inst, 31);

  if ((opc == 3) && (!if_64bit & (N == 1)) && (if_64bit & (N == 0))) {
    unallocated();
    return;
  }
  if (imms >= immr) {
    len = imms - immr + 1;
    from = immr;
    to = 0;
  } else {
    len = imms + 1;
    from = 0;
    to = if_64bit ? 64 - immr : 32 - immr;
  }
  imm = util::shift(xregs[rn], from, from + len - 1) << to;

  switch (opc) {
  case 0:
    LOG_CPU("SBFM, to=%d, from=%d, len=%d\n", to, from, len);
    xregs[rd] = 0;
    xregs[rd] = signed_extend(imm, /*topbit=*/to + len - 1);
    break;
  case 1:
    LOG_CPU("BFM\n");
    xregs[rd] = imm | (xregs[rd] & ((1 << to) - 1)) |
                (xregs[rd] & (util::mask(len) << to));
    break;
  case 2:
    LOG_CPU("UBFM\n");
    xregs[rd] = imm;
    break;
  default:
    assert(false);
  }
}

void Cpu::decode_extract(uint32_t inst) { LOG_CPU("%d\n", inst); }

/*
====================================================
         Loads and stores
====================================================
*/

/*
         Load/store register pair(offset/post-indexed/pre-indexed)

         31   30 29 27  26 25 23  22 21         15 14  10 9    5 4     0
         +-----+------+---+-----+---+-------------+------+---- -+------+
         | opc |  101 | V | opt | L |    imm7     |  Rt2 |  Rn  |  Rt  |
         +-----+------+---+-----+---+-------------+------+---- -+------+

  @opc: 00:32bit, 01:?, 10:64bit, 11:unallocated
  @V: if vector
  @opt: 000:no-allocate, 001:post-indexed, 010:offset, 011:pre-indexed
  @L: if load
*/
void Cpu::decode_ldst_register_pair(uint32_t inst) {
  bool /*if_vector, */ wback, postindex, if_32bit;
  uint8_t if_load, opc, opt, rn, rt, rt2;
  uint64_t data1, data2, imm7, offset;
  int64_t address;
  const char *op;

  opc = util::shift(inst, 30, 31);
  // if_vector = util::bit(inst, 26);
  opt = util::shift(inst, 23, 25);
  if_load = util::bit(inst, 22);
  imm7 = util::shift(inst, 15, 21);
  rt2 = util::shift(inst, 10, 14);
  rn = util::shift(inst, 5, 9);
  rt = util::shift(inst, 0, 4);

  if_32bit = 0;

  switch (opc) {
  case 0:
    if_32bit = 1;
    if (if_load) {
      op = "ldp";
      unsupported();
      return;
    } else {
      op = "stp";
      unsupported();
      return;
    }
    break;
  case 1:
    if (if_load) {
      op = "ldpsw";
      unsupported();
      return;
    } else {
      op = "ldpstgp";
      unsupported();
      return;
    }
    break;
  case 2:
    if (if_load) {
      op = "ldp";
    } else {
      op = "stp";
    }
    break;
  case 3:
    unallocated();
    break;
  default:
    assert(false);
  }

  offset = util::SIGN_EXTEND(imm7, 7);
  if (if_32bit) {
    offset = offset << 2;
  } else {
    offset = offset << 3;
  }

  switch (opt) {
  case 0:
    unsupported();
    break;
  case 1:
    wback = true;
    postindex = true;
    LOG_CPU("%s x%d, x%d, [x%d], #%ld\n", op, rt, rt2, rn, (int64_t)offset);
    break;
  case 2:
    wback = false;
    postindex = false;
    LOG_CPU("%s x%d, x%d, [x%d, #%ld]\n", op, rt, rt2, rn, (int64_t)offset);
    break;
  case 3:
    wback = true;
    postindex = false;
    LOG_CPU("%s x%d, x%d, [x%d, #%ld]!\n", op, rt, rt2, rn, (int64_t)offset);
    break;
  default:
    unallocated();
  }

  address = xregs[rn];
  if (!postindex) {
    address += (int64_t)offset;
  }

  if (if_load) {
    if (if_32bit) {
      xregs[rt] = bus.load32(address);
      xregs[rt2] = bus.load32(address + 4);
    } else {
      xregs[rt] = bus.load64(address);
      xregs[rt2] = bus.load64(address + 8);
    }
  } else {
    data1 = xregs[rt];
    data2 = xregs[rt2];
    if (if_32bit) {
      bus.store32(address, data1);
      bus.store32(address + 4, data2);
    } else {
      bus.store64(address, data1);
      bus.store64(address + 8, data2);
    }
  }

  if (wback) {
    if (postindex) {
      address += offset;
    }
    xregs[rn] = address;
  }
}

void Cpu::decode_ldst_register(uint32_t inst) {
  uint8_t op0, op1, op2, op3, op4, op;
  op0 = util::shift(inst, 28, 31);
  op1 = util::bit(inst, 26);
  op2 = util::shift(inst, 23, 24);
  op3 = util::shift(inst, 16, 21);
  op4 = util::shift(inst, 10, 11);
  switch (op0 % 4) {
  case 0:
    unsupported();
    return;
  case 1:
    unsupported();
    return;
  case 2:
    unsupported();
    return;
  case 3:
    if (op2 >= 2) {
      decode_ldst_reg_unsigned_imm(inst);
    } else {
      op = (util::bit(inst, 21)) << 2 | util::shift(inst, 10, 11);
      const decode_func decode_ldst_reg_tbl[] = {
          &Cpu::decode_ldst_reg_immediate,     &Cpu::decode_ldst_reg_immediate,
          &Cpu::decode_ldst_reg_unpriviledged, &Cpu::decode_ldst_reg_immediate,
          &Cpu::decode_ldst_atomic_memory_op,  &Cpu::decode_ldst_reg_pac,
          &Cpu::decode_ldst_reg_reg_offset,    &Cpu::decode_ldst_reg_pac,
      };
      (this->*decode_ldst_reg_tbl[op])(inst);
    }
    return;
  }
}

/*
         Load/store register (unsigned immediate)

         31   30 29 27  26 25 24 23 22 21         10  9    5 4   0
         +------+-----+---+----+------+--------------+------+-----+
         | size | 111 | V | 01 | opc  |    imm12     |  Rn  |  Rt |
         +------+-----+---+----+------+--------------+------+-----+

         @size: 00:8bit, 01:16bit, 10:32bit, 11:64bit
         @V: simd
         @Rm: offset register
         @opt: extend type
         @S: if S=1 then shift |size|
         @idx: 00:unscaled immediate, 01:post-indexed, 11:pre-indexed
         @Rn: base register or stack pointer
         @Rt: register to be transfered

*/
void Cpu::decode_ldst_reg_unsigned_imm(uint32_t inst) {
  bool vector;
  uint8_t size, opc, rn, rt;
  uint64_t imm12, offset;

  size = util::shift(inst, 30, 31);
  vector = util::bit(inst, 26);
  opc = util::shift(inst, 22, 23);
  imm12 = util::shift(inst, 10, 21);
  rn = util::shift(inst, 5, 9);
  rt = util::shift(inst, 0, 4);

  if (vector) {
    printf("ldst_reg_unsigned_imm\n");
    unsupported();
    return;
  }

  switch (size) {
  case 0:
  case 1:
    printf("ldr/str 8/16\n");
    unsupported();
    return;
  case 2:
    offset = imm12 << size;
    switch (opc) {
    case 0:
      bus.store32(xregs[rn] + offset, xregs[rt]);
      LOG_CPU("str x%d, [x%d, #%ld]\n", rt, rn, offset);
      break;
    case 1:
      xregs[rt] = util::set_lower32(xregs[rt], bus.load32(xregs[rn] + offset));
      LOG_CPU("ldr x%d, [x%d, #%ld]\n", rt, rn, offset);
      break;
    case 2:
      LOG_CPU("ldrsw\n");
      break;
    default:
      assert(false);
    }
    return;
  case 3:
    offset = imm12 << size;
    switch (opc) {
    case 0:
      bus.store64(xregs[rn] + offset, xregs[rt]);
      LOG_CPU("str x%d, [x%d, #%ld]\n", rt, rn, offset);
      break;
    case 1:
      xregs[rt] = bus.load64(xregs[rn] + offset);
      LOG_CPU("ldr x%d, [x%d, #%ld]\n", rt, rn, offset);
      break;
    case 2:
      printf("prfm\n");
      unsupported();
      return;
    }
    break;
  default:
    assert(false);
  }
}

/*
         Load/store register (unscaled immediate)
         Load/store register (immediate pre-indexed)

         31   30 29 27 26  25 24 23 22 21  20      12 11  10 9   5 4   0
         +------+-----+---+----+------+---+----------+------+-----+-----+
         | size | 111 | V | 00 | opc  | 0 |   imm9   | idx  |  Rn |  Rt |
         +------+-----+---+----+------+---+----------+------+-----+-----+

         @size: 00:8bit, 01:16bit, 10:32bit, 11:64bit
         @V: simd
         @Rm: offset register
         @opt: extend type
         @S: if S=1 then shift |size|
         @idx: 00:unscaled immediate, 01:post-indexed, 11:pre-indexed,
   10:unprivileged
         @Rn: base register or stack pointer
         @Rt: register to be transfered

*/

void Cpu::decode_ldst_reg_immediate(uint32_t inst) {
  bool vector;
  uint8_t size, opc, rn, rt, idx;
  uint64_t imm9, offset;
  bool post_indexed, writeback;

  size = util::shift(inst, 30, 31);
  vector = util::bit(inst, 26);
  opc = util::shift(inst, 22, 23);
  imm9 = util::shift(inst, 12, 20);
  idx = util::shift(inst, 10, 11);
  rn = util::shift(inst, 5, 9);
  rt = util::shift(inst, 0, 4);

  switch (idx) {
  case 0:
    post_indexed = false;
    writeback = false;
    break;
  case 1:
    post_indexed = true;
    writeback = true;
    break;
  case 3:
    post_indexed = false;
    writeback = true;
    break;
  case 2:
    // see ldst_reg_unprivileged
    assert(false);
  }

  if (vector) {
    unsupported();
  } else {

    if (post_indexed) {
      offset = 0;
      printf("post_indexed\n");
    } else {
      offset = util::SIGN_EXTEND(imm9, 9);
      printf("offset=0x%lx\n", offset);
    }

    switch (opc) {
    case 0b00:
      /* STR */
      LOG_CPU("stur size=%d, rt=%d, rn=%d, "
              "offset=%lu\n",
              size_tbl[size], rt, rn, offset);
      // TODO size
      bus.store64(xregs[rn] + offset, xregs[rt]);
      break;
    case 0b01:
      /* LDR (unsigned) */
      LOG_CPU("ldur, x%d, x%d, #0x%lx\n", rt, rn, offset);
      if (size == 3) {
        // TODO size
        xregs[rt] = bus.load64(xregs[rn] + offset);
      } else {
        // TODO size
        update_lower32(rt, bus.load64(xregs[rn] + offset));
      }
      break;
    case 0b10:
      /* LDR (signed 64bit) */
      if (size == 3) {
        unsupported();
        break;
      }
      LOG_CPU("load_store: register: LDR(signed 64bit): size=%d, rt=%d, rn=%d, "
              "offset=%lu\n",
              size_tbl[size], rt, rn, offset);
      xregs[rt] = signed_extend(
          // TODO size
          bus.load64(xregs[rn] + offset), size_tbl[size]);
      break;
    case 0b11:
      /* LDR (signed 32bit) */
      if (size >= 2) {
        unallocated();
      }
      LOG_CPU("load_store: register: LDR(signed 32bit): size=%d, rt=%d, rn=%d, "
              "offset=%lu\n",
              size_tbl[size], rt, rn, offset);
      update_lower32(
          rt,
          // TODO size
          signed_extend32(bus.load64(xregs[rn] + offset), size_tbl[size]));
      break;
    }

    if (writeback) {
      LOG_CPU("\twriteback address\n");
      xregs[rn] = xregs[rn] + offset;
    }
  }
}

void Cpu::decode_ldst_reg_unpriviledged(uint32_t inst) {
  LOG_CPU("load_store: ldst_reg_unpriviledged\n");
  LOG_CPU("%d\n", inst);
}
void Cpu::decode_ldst_atomic_memory_op(uint32_t inst) {
  LOG_CPU("load_store: ldst_reg_atomic_memory_op\n");
  LOG_CPU("%d\n", inst);
}
void Cpu::decode_ldst_reg_pac(uint32_t inst) {
  LOG_CPU("load_store: ldst_reg_pca\n");
  LOG_CPU("%d\n", inst);
}

// ExtendReg() in ARM
// Perform a value extension and shift
static uint64_t shift_and_extend(uint64_t val, bool shift, uint8_t scale,
                                 ExtendType exttype) {
  bool if_unsigned = false;
  uint8_t len = 0;

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
  if (if_unsigned) {
    return val;
  } else {
    return signed_extend(val, len);
  }
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
void Cpu::decode_ldst_reg_reg_offset(uint32_t inst) {
  bool vector, shift;
  uint8_t size, scale, opc, opt, rm, rn, rt;
  uint64_t offset;

  size = util::shift(inst, 30, 31);
  scale = size;
  vector = util::bit(inst, 26);
  opc = util::shift(inst, 22, 23);
  rm = util::shift(inst, 16, 20);
  opt = util::shift(inst, 13, 15);
  shift = util::bit(inst, 12);
  rn = util::shift(inst, 5, 9);
  rt = util::shift(inst, 0, 4);

  if (vector) {
    unsupported();
  } else {
    switch (opc) {
    case 0b00:
      /* store */
      // TODO: option = 0b011
      offset = shift_and_extend(xregs[rm], shift, scale, extendtype_tbl[opt]);
      // TODO size
      bus.store64(xregs[rn] + offset, xregs[rt]);
      LOG_CPU("load_store: register offset: store: rt %d [rn] %lu offset %lu\n",
              rt, xregs[rn], offset);
      break;
    case 0b11: /* loads */
      if (size >= 0b10) {
        unallocated();
      }
      break;
    case 0b01: /* loadu */
    case 0b10: /* loads */
               // TODO: option = 0b011
      offset = shift_and_extend(xregs[rm], shift, scale, extendtype_tbl[opt]);
      LOG_CPU("load_store: register offset: load: rt=%d, rn=%d, [rn]=0x%lx, "
              "rm=%d, "
              "[rm]=0x%lx, offset=%lu vaddr=0x%lx\n",
              rt, rn, xregs[rn], rm, xregs[rm], offset, xregs[rn] + offset);
      // TODO size
      xregs[rt] = bus.load64(xregs[rn] + offset);
      break;
    }
  }
}

/*
====================================================
         Data Processing Register
====================================================
*/

/*
         Add/substruct (shifted register)
           31   30   29 28   24 2322  21  20   16 15  10 9   5 4   0
         +----+----+---+-------+----+---+-------+------+-----+-----+
         | sf | op | S | 01011 | sh | 0 |  Rm   | imm6 |  Rn |  Rd |
         +----+----+---+-------+----+---+-------+------+-----+-----+

         @sf: 0->32bit, 1->64bit
         @op: 0->ADD, 1->SUB
         @S: set flag
         @sh: if sh=1 then LSL #12
         @imm6: shift amount
         @Rn: source gp regiter or sp
*/
void Cpu::decode_addsub_shifted_reg(uint32_t inst) {
  uint8_t shift_type, rd, rn, rm, shift_amount;
  uint64_t op1, op2, result;
  bool if_sub, if_setflag, if_64bit;
  const char *op;

  if_64bit = util::bit(inst, 31);
  if_sub = util::bit(inst, 30);
  if_setflag = util::bit(inst, 29);
  shift_type = util::shift(inst, 22, 23);
  rm = util::shift(inst, 16, 20);
  shift_amount = util::shift(inst, 10, 15);
  rn = util::shift(inst, 5, 9);
  rd = util::shift(inst, 0, 4);
  op = if_sub ? "sub" : "add";

  if (shift_type == 3 || (!if_64bit & (shift_amount >= 0b100000))) {
    unallocated();
  }

  op1 = xregs[rn];
  op2 = util::shift_with_type(xregs[rm], shift_type, shift_amount);

  if (if_setflag) {
    result = add_imm_s(op1, op2, if_sub, cpsr, if_64bit);
    LOG_CPU("%ss x%d, x%d, x%d, %s #%d\n", op, rd, rn, rm,
            shift_type_strtbl[shift_type], shift_amount);
  } else {
    result = add_imm(op1, op2, if_sub);
    LOG_CPU("%s x%d, x%d, x%d, %s #%d\n", op, rd, rn, rm,
            shift_type_strtbl[shift_type], shift_amount);
  }

  xregs[rd] = if_64bit ? result : util::set_lower32(xregs[rd], result);
}

/*
         Add/substruct (extended register)
           31   30   29 28   24 2322  21  20  16 15    13 12  10 9   5 4   0
         +----+----+---+-------+----+---+-------+--------+------+-----+-----+
         | sf | op | S | 01011 | opt| 1 |  Rm   | option | imm3 | Rn  |  Rd |
         +----+----+---+-------+----+---+-------+--------+------+-----+-----+

         @sf: 0->32bit, 1->64bit
         @op: 0->ADD, 1->SUB
         @S: set flag
         @opt: 01,10,11->unallocated
         @option: extend type
         @imm3: shift amount
*/
void Cpu::decode_addsub_extended_reg(uint32_t inst) {
  uint8_t opt, shift_amount, extend_type, rm, rn, rd;
  uint64_t op1, op2, result;
  bool if_64bit, if_sub, if_setflag;
  const char *op;

  if_64bit = util::bit(inst, 31);
  if_sub = util::bit(inst, 30);
  if_setflag = util::bit(inst, 29);
  opt = util::shift(inst, 22, 23);
  rm = util::shift(inst, 16, 20);
  extend_type = util::shift(inst, 13, 15);
  shift_amount = util::shift(inst, 10, 12);
  rn = util::shift(inst, 5, 9);
  rd = util::shift(inst, 0, 4);
  op = if_sub ? "sub" : "add";

  if (opt || (shift_amount > 4)) {
    unallocated();
    return;
  }

  op1 = xregs[rn];
  op2 = ExtendValue(xregs[rm], extend_type, shift_amount);
  result = if_setflag ? add_imm_s(op1, op2, if_sub, cpsr, if_64bit)
                      : add_imm(op1, op2, if_sub);
  xregs[rd] = if_64bit ? result : util::set_lower32(xregs[rd], result);
  LOG_CPU("%s x%d(0x%lx), x%d(0x%lx), x%d(0x%lx)\n", op, rd, xregs[rd], rn,
          xregs[rn], rm, xregs[rm]);
  return;
}

/*
         Logical (shifted register)
           31  30 29 28   24 2322 21 20   16 15   10 9    5 4   0
         +----+-----+-------+----+---+-------+-------+------+-----+
         | sf | opc | 01010 | sh | N |  Rm   | imm6  |  Rn  |  Rd |
         +----+-----+-------+----+---+-------+-------+------+-----+

         @sf: 0->32bit, 1->64bit
         @opc: 0->AND, 1->OR, 2->EOR, 3->ANDS
         @sh: 00->LSL, 01->LSR, 10->ASR, 11->ROR
         @N: not
         @Rn: source gp regiter or sp
*/
void Cpu::decode_logical_shifted_reg(uint32_t inst) {
  uint8_t opc, rd, rn, rm, imm6, shift;
  uint64_t op1, op2;
  bool if_64bit;
  // bool setflag, if_64bit, if_not;

  if_64bit = util::bit(inst, 31);
  opc = util::shift(inst, 29, 30);
  shift = util::shift(inst, 22, 23);
  // if_not = util::bit(inst, 21);
  rm = util::shift(inst, 16, 20);
  imm6 = util::shift(inst, 10, 15);
  rn = util::shift(inst, 5, 9);
  rd = util::shift(inst, 0, 4);

  op1 = rn == 31 ? 0 : xregs[rn];

  op2 = xregs[rm];
  switch (shift) {
  case 0:
    op2 = op2 << imm6;
    break;
  case 1:
    op2 = op2 >> imm6;
    break;
  default:
    unsupported();
  }

  uint64_t result;
  switch (opc) {
  case 1:
    result = op1 | op2;
    xregs[rd] = if_64bit
                    ? result
                    : (~util::mask(32) & xregs[rd]) | (util::mask(32) & result);
    LOG_CPU("orr x%d, 0x%lx | 0x%lx\n", rd, op1, op2);
    break;
  default:
    printf("decode_logical_shifted_reg\n");
    unsupported();
    break;
  }
}

/*
         Conditional Select
           31   30  29  28      21 20  1615  12 11 10 9   5 4   0
         +----+----+---+----------+-----+------+-----+-----+-----+
         | sf | op | S | 11010100 | Rm  | cond | op2 |  Rn |  Rd |
         +----+----+---+----------+-----+------+-----+-----+-----+

         @sf: 0->32bit, 1->64bit
         @opc: 0->AND, 1->OR, 2->EOR, 3->ANDS
         @sh: 00->LSL, 01->LSR, 10->ASR, 11->ROR
         @N: not
         @Rn: source gp regiter or sp
*/
void Cpu::decode_conditional_select(uint32_t inst) {
  uint64_t result;
  uint8_t cond, op1, op2, rm, rn, rd;
  bool if_64bit, s;

  if_64bit = util::bit(inst, 31);
  op1 = util::bit(inst, 30);
  s = util::bit(inst, 29);
  rm = util::shift(inst, 16, 20);
  cond = util::shift(inst, 12, 15);
  op2 = util::shift(inst, 10, 11);
  rn = util::shift(inst, 5, 9);
  rd = util::shift(inst, 0, 4);
  if (s || op2 >= 2) {
    unallocated();
    return;
  }
  switch (op1) {
  case 0:
    switch (op2) {
    case 0:
      LOG_CPU("csel\n");
      unsupported();
      break;
    case 1:
      result = check_b_flag(cond) ? xregs[rn] : xregs[rm] + 1;
      xregs[rd] = result;
      LOG_CPU("csinc x%d, x%d, x%d\n", rd, rn, rm);
      break;
    default:
      assert(false);
    }
    break;
  case 1:
    switch (op2) {
    case 0:
      LOG_CPU("csinv\n");
      unsupported();
      break;
    case 1:
      LOG_CPU("csneg\n");
      unsupported();
      break;
    default:
      assert(false);
    }
    break;
  default:
    assert(false);
  }
}

/*
===========================================================
   Branches, Exception Generating and Cpu instructions
===========================================================
*/

/*
         Conditional branch (immediate)

           31    25  24  23                   5  4    3   0
         +---------+----+----------------------+----+------+
         | 0101010 | o1 |        imm19         | o0 | cond |
         +---------+----+----------------------+----+------+

         @o1: 1->unallocated
         @o0: 0->B.cond, 1->BC.cond

                                 BC.cond:
                                 - "Branch Consistent conditionally to a label
   at a PC-relative offset, with a hint that this branch will behave very
   consistently and is very unlikely to change direction."
                                 - same as B.cond in this emulator.
*/
bool Cpu::check_b_flag(uint8_t cond) {
  switch (cond) {
  case 0:
    return cpsr.Z == 1;
    break;
  case 1:
    return cpsr.Z == 0;
    break;
  case 2:
    return cpsr.C == 1;
    break;
  case 3:
    return cpsr.C == 0;
    break;
  case 4:
    return cpsr.N == 1;
    break;
  case 5:
    return cpsr.N == 0;
    break;
  case 6:
    return cpsr.V == 1;
    break;
  case 7:
    return cpsr.V == 0;
    break;
  case 8:
    return (cpsr.C == 1) & (cpsr.Z == 0);
    break;
  case 9:
    return (cpsr.C == 0) | (cpsr.Z == 1);
    break;
  case 10:
    return cpsr.N == cpsr.V;
    break;
  case 11:
    return cpsr.N != cpsr.V;
    break;
  case 12:
    return (cpsr.Z == 0) & (cpsr.N == cpsr.V);
    break;
  case 13:
    return (cpsr.Z == 1) | (cpsr.N != cpsr.V);
    break;
  case 14:
    return true;
    break;
  case 15:
    return true;
    break;
  }
  return false;
}

void Cpu::decode_conditional_branch_imm(uint32_t inst) {
  uint64_t offset;
  uint32_t imm19;
  uint8_t o1, o0, cond;

  o1 = util::bit(inst, 24);
  imm19 = util::shift(inst, 5, 23);
  o0 = util::bit(inst, 4);
  cond = util::shift(inst, 0, 3);

  if (o1 == 1) {
    unallocated();
  }

  if (o0 == 0) {
    if (check_b_flag(cond)) {
      offset = signed_extend(imm19 << 2, 20);
      LOG_CPU("B.cond: pc=0x%lx offset=0x%lx, cond=0x%x\n", pc + offset, offset,
              cond);
      set_pc(pc + offset);
      return;
    }
  }
  increment_pc();
}

/*
         Exception generation(C4-528)

          31      24  23 21  20             5 4   2 1  0
         +----------+-------+----------------+-----+----+
         | 11010100 |  opc  |    imm16       | op2 | LL |
         +----------+-------+----------------+-----+----+

         @op:
*/
void Cpu::decode_exception_generation(uint32_t inst) {
  uint8_t opc, op2, LL;
  // uint64_t imm16;

  opc = util::shift(inst, 21, 23);
  // imm16 = util::shift(inst, 5, 20);
  op2 = util::shift(inst, 2, 4);
  LL = util::shift(inst, 0, 1);

  switch (opc) {
  case 0:
    if (op2 != 0) {
      unallocated();
      break;
    }
    switch (LL) {
    case 1:
      LOG_CPU("SVC: w8=%ld, w0=%ld, w1=0x%lx, w2=%ld\n", xregs[8], xregs[0],
              xregs[1], xregs[2]);
      write(xregs[0], (void *)xregs[1], xregs[2]);
      break;
    case 2:
      LOG_CPU("HVC\n");
      break;
    case 3:
      LOG_CPU("SMC\n");
      break;
    default:
      unallocated();
    }
    break;
  default:
    unsupported();
    break;
  }
  increment_pc();
}

/*
         Unconditional branch (reg)

          31     25  24  21 20  16  15   10 9    5  4    0
         +---------+-------------------------------------+
         | 1101011 |  opc  |  op2  |  op3  |  Rn  |  op4  |
         +---------+-------------------------------------+

         @op:
*/
void Cpu::decode_unconditional_branch_reg(uint32_t inst) {
  uint8_t opc, /*op2,*/ op3, Rn, op4, n;
  uint64_t target = pc;

  opc = util::shift(inst, 21, 24);
  // op2 = util::shift(inst, 16, 20);
  op3 = util::shift(inst, 10, 15);
  Rn = util::shift(inst, 5, 9);
  op4 = util::shift(inst, 0, 4);

  switch (opc) {
  case 2:
    switch (op3) {
    case 0:
      if (op4 != 0) {
        unallocated();
        break;
      }
      // RET
      n = (Rn == 0) ? 30 : Rn;
      assert(n <= 31);
      target = xregs[n];
      LOG_CPU("RET: target=xregs[%d](0x%lx)\n", n, target);
      if (target == 0) {
        exit(0);
      }
      break;
    default:
      unsupported();
    }
    break;
  default:
    unsupported();
  }
  set_pc(target);
}

/*
         Unconditional branch (immediate)

           31   30  26 25                                  0
         +----+-------+------------------------------------+
         | op | 00101 |             imm26                  |
         +----+-------+------------------------------------+

         @op: 0->Branch, 1->Branch with Link

*/
void Cpu::decode_unconditional_branch_imm(uint32_t inst) {
  uint64_t offset;
  uint32_t imm26;
  uint8_t op;

  op = util::bit(inst, 31);
  imm26 = util::shift(inst, 0, 25);

  offset = signed_extend(imm26 << 2, 27);

  switch (op) {
  case 0:
    LOG_CPU("b 0x%lx\n", pc + offset);
    break;
  case 1:
    xregs[30] = pc + 4;
    LOG_CPU("bl 0x%lx\n", pc + offset);
    break;
  }
  set_pc(pc + offset);
}

/*
         Compare and branch (immediate)

           31   30   25  24   23                 5  4      0
         +----+--------+----+-------------------------------+
         | sh | 011010 | op |          imm19       |   Rt   |
         +----+--------+----+-------------------------------+

         @sh: 0->32bit, 1->64bit
                                 @op: 0->CBZ, 1->CBNZ

*/
void Cpu::decode_compare_and_branch_imm(uint32_t inst) {
  uint64_t offset;
  uint32_t imm19;
  uint8_t op, if_64bit, rt;
  bool if_jump = false;

  if_64bit = util::bit(inst, 31);
  op = util::bit(inst, 24);
  imm19 = util::shift(inst, 5, 23);
  rt = util::shift(inst, 0, 4);

  switch (op) {
  case 0: // CBZ
    if (if_64bit) {
      if_jump = xregs[rt] == 0;
    } else {
      if_jump = (uint32_t)xregs[rt] == 0;
    }
    break;
  case 1: // CBNZ
    if (if_64bit) {
      if_jump = xregs[rt] != 0;
    } else {
      if_jump = (uint32_t)xregs[rt] != 0;
    }
    break;
  }

  if (if_jump) {
    offset = signed_extend(imm19 << 2, 20);
    set_pc(pc - 4 + offset);
  }
}
