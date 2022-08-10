#include "system.h"

#include <cassert>
#include <iostream>
#include <memory>

#include <stdio.h>

#include "arm.h"
#include "log.h"
#include "mem.h"
#include "utils.h"

namespace core {

typedef __attribute__((mode(TI))) unsigned int uint128_t;
typedef __attribute__((mode(TI))) int int128_t;

System::System(const char *filename, const uint64_t initaddr)
    : cpu_(cpu::Cpu(this)), mem_(mem::Mem(this)), initaddr_(initaddr),
      filename_(filename) {}

System::~System() {
  // free
  mem_.clean_mem();
}

SystemResult System::Init() {
  SystemResult err;

  printf("emu: start emulating\n");

  /// mem
  err = mem_.init_mem(filename_, initaddr_);
  if (err != SystemResult::Success) {
    fprintf(stderr, "emu: failed to initialize mem\n");
    return SystemResult::ErrorMemory;
  }

  return SystemResult::Success;
}

uint32_t System::fetch() { return mem_.read_inst(cpu_.pc); }

void System::execute_loop() {
  uint32_t inst;

  while (cpu_.pc < (uint64_t)mem_.text_end) {
    inst = fetch();
    decode_start(inst);
  }
}

namespace {

const int size_tbl[] = {8, 16, 32, 64};

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
static void unsupported() { LOG_CPU("unsuported inst\n"); }

static void unallocated() { LOG_CPU("unallocated inst\n"); }

static inline uint64_t bitmask64(uint8_t length) {
  return ~0ULL >> (64 - length);
}

static inline uint64_t signed_extend(uint64_t val, uint8_t topbit) {
  return bitutil::bit(val, topbit) ? (val | ~uint64_t(0) << topbit) : val;
}

static inline uint32_t signed_extend32(uint32_t val, uint8_t topbit) {
  return bitutil::bit(val, topbit) ? (val | ~uint32_t(0) << topbit) : val;
}

} // namespace

void System::decode_start(uint32_t inst) {
  uint8_t op1;
  op1 = bitutil::shift(inst, 25, 28);

  LOG_CPU("PC=0x%lx\n", cpu_.pc);
  const decode_func decode_inst_tbl[] = {
      &System::decode_sme_encodings,
      &System::decode_unallocated,
      &System::decode_sve_encodings,
      &System::decode_unallocated,
      &System::decode_loads_and_stores,
      &System::decode_data_processing_reg,
      &System::decode_loads_and_stores,
      &System::decode_data_processing_float,
      &System::decode_data_processing_imm,
      &System::decode_data_processing_imm,
      &System::decode_branches,
      &System::decode_branches,
      &System::decode_loads_and_stores,
      &System::decode_data_processing_reg,
      &System::decode_loads_and_stores,
      &System::decode_data_processing_float,
  };
  (this->*decode_inst_tbl[op1])(inst);
}

void System::decode_data_processing_imm(uint32_t inst) {
  uint8_t op0;
  op0 = bitutil::shift(inst, 23, 25);
  const decode_func decode_data_processing_imm_tbl[] = {
      &System::decode_pc_rel,      &System::decode_pc_rel,
      &System::decode_add_sub_imm, &System::decode_add_sub_imm_with_tags,
      &System::decode_logical_imm, &System::decode_move_wide_imm,
      &System::decode_bitfield,    &System::decode_extract,
  };
  (this->*decode_data_processing_imm_tbl[op0])(inst);
  cpu_.increment_pc();
}

void System::decode_loads_and_stores(uint32_t inst) {
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
    System::decode_ldst_register_pair(inst);
    break;
  case 0b11:
    System::decode_ldst_register(inst);
    break;
  }
  cpu_.increment_pc();
}

void System::decode_data_processing_reg(uint32_t inst) {
  uint8_t op1, op2;

  op1 = bitutil::bit(inst, 28);
  op2 = bitutil::shift(inst, 21, 24);
  LOG_CPU("data_processing_reg, op1=0x%x, op2=0x%x\n", op1, op2);
  if ((op1 == 0) && (op2 == 8)) {
    decode_addsub_shifted_reg(inst);
  } else if ((op1 == 0) && (op2 < 8)) {
    decode_logical_shifted_reg(inst);
  } else {
    unsupported();
  }
  cpu_.increment_pc();
}

void System::decode_data_processing_float(uint32_t inst) {
  LOG_CPU("data_processing_float %d\n", inst);
  cpu_.increment_pc();
}

void System::decode_branches(uint32_t inst) {
  uint8_t op;
  op = bitutil::shift(inst, 29, 31);
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
    if (bitutil::bit(inst, 25) == 0) {
      decode_compare_and_branch_imm(inst);
    } else {
      unsupported();
    }
    break;
  case 6:
    if (bitutil::bit(inst, 25) == 1) {
      decode_unconditional_branch_reg(inst);
    } else {
      unsupported();
    }
    break;
  default:
    unsupported();
    break;
  }
}
void System::decode_pc_rel(uint32_t inst) {
  LOG_CPU("pc_rel %d\n", inst);
  cpu_.increment_pc();
}

void System::decode_sme_encodings(uint32_t inst) {
  LOG_CPU("sme_encodings %d\n", inst);
  cpu_.increment_pc();
}

void System::decode_unallocated(uint32_t inst) {
  LOG_CPU("unallocated %d\n", inst);
  cpu_.increment_pc();
}

void System::decode_sve_encodings(uint32_t inst) {
  LOG_CPU("sve_encodings %d\n", inst);
  cpu_.increment_pc();
}

/*
====================================================
         Data Processing Immediate
====================================================
*/

static uint64_t add_imm(uint64_t x, uint64_t y, uint8_t carry_in) {
  return x + y + carry_in;
}

static uint32_t add_imm_s32(uint32_t x, uint32_t y, uint8_t carry_in,
                            core::cpu::CPSR &cpsr) {
  int32_t sx = (int32_t)x;
  int32_t sy = (int32_t)y;
  uint64_t unsigned_sum = (uint64_t)x + (uint64_t)y + (uint64_t)carry_in;
  int64_t signed_sum = (int64_t)sx + (int64_t)sy + (uint64_t)carry_in;
  uint32_t result = unsigned_sum & bitutil::mask(32);

  cpsr.N = bitutil::bit(result, 31);
  cpsr.Z = result == 0;
  cpsr.C = unsigned_sum != result;
  cpsr.V = signed_sum != (int32_t)result;
  return result;
}

static uint64_t add_imm_s(uint64_t x, uint64_t y, uint8_t carry_in,
                          core::cpu::CPSR &cpsr, bool if_64bit) {
  if (!if_64bit) {
    return add_imm_s32(x, y, carry_in, cpsr);
  }
  int64_t sx = (int64_t)x;
  int64_t sy = (int64_t)y;
  uint128_t unsigned_sum = (uint128_t)x + (uint128_t)y + (uint128_t)carry_in;
  int128_t signed_sum = (int128_t)sx + (int128_t)sy + (uint128_t)carry_in;
  uint64_t result = unsigned_sum & bitutil::mask(64);

  cpsr.N = bitutil::bit64(result, 63);
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
void System::decode_add_sub_imm(uint32_t inst) {
  uint8_t rd, rn;
  uint64_t imm;
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
      LOG_CPU("SUBS: rd=%d, xregs[rn]=%lu, imm=0x%lx\n", rd, cpu_.xregs[rn],
              ~imm);
      result =
          add_imm_s(cpu_.xregs[rn], ~imm, /*carry-in=*/1, cpu_.cpsr, if_64bit);
    } else {
      /* ADDS */
      LOG_CPU("ADDS: rd=%d, xregs[rn]=%lu, imm=0x%lx, if_64bit=%d\n", rd,
              cpu_.xregs[rn], imm, if_64bit);
      result =
          add_imm_s(cpu_.xregs[rn], imm, /*carry-in=*/0, cpu_.cpsr, if_64bit);
    }
    if (rd == 31) {
      return;
    }
  } else {
    if (if_sub) {
      /* SUB */
      LOG_CPU("SUB: rd=%d, xregs[rn]=%lu, imm=0x%lx\n", rd, cpu_.xregs[rn],
              ~imm);
      result = add_imm(cpu_.xregs[rn], ~imm, /*carry-in=*/1);
    } else {
      /* ADD */
      LOG_CPU("ADD: rd=%d, xregs[rn]=%lu, imm=0x%lx\n", rd, cpu_.xregs[rn],
              imm);
      result = add_imm(cpu_.xregs[rn], imm, /*carry-in=*/0);
    }
  }

  if (if_64bit) {
    cpu_.xregs[rd] = result;
  } else {
    cpu_.xregs[rd] = bitutil::mask(32) & result;
  }
}

void System::decode_add_sub_imm_with_tags(uint32_t inst) {
  LOG_CPU("%d\n", inst);
}

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

void System::decode_logical_imm(uint32_t inst) {
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

  if (!if_64bit & (N == 1)) {
    unallocated();
    return;
  }

  switch (opc) {
  case 0b00:
    LOG_CPU("AND: [rn]=%lu, imm=%d\n", cpu_.xregs[rn], imm);
    result = cpu_.xregs[rn] & imm; /* AND */
    break;
  case 0b01:
    LOG_CPU("ORR: [rn]=%lu, imm=%d\n", cpu_.xregs[rn], imm);
    result = cpu_.xregs[rn] | imm; /* ORR */
    break;
  case 0b10:
    LOG_CPU("EOR: [rn]=%lu, imm=%d\n", cpu_.xregs[rn], imm);
    result = cpu_.xregs[rn] ^ imm; /* EOR */
    break;
  case 0b11:
    LOG_CPU("ANDS: [rn]=%lu, imm=%d\n", cpu_.xregs[rn], imm);
    result = cpu_.xregs[rn] & imm; /* ANDS */
    if (if_64bit) {
      cpu_.cpsr.N = (int64_t)result < 0;
    } else {
      cpu_.cpsr.N = (int32_t)result < 0;
    }
    cpu_.cpsr.Z = result == 0;
    cpu_.cpsr.C = 0;
    cpu_.cpsr.V = 0;
    break;
  default:
    assert(false);
  }

  if (if_64bit) {
    cpu_.xregs[rd] = result;
  } else {
    cpu_.xregs[rd] = bitutil::clear_upper32(result);
  }
}

/*
         Move wide (immediate)

           31   30 29 28    23 22 21 20           5 4   0
         +----+------+--------+-----+--------------+-----+
         | sf |  opc | 100101 |  hw |     imm16    |  Rd |
         +----+------+--------+-----+--------------+-----+

         @sf: 0->32bit, 1->64bit
         @opc: 0->N, 1->unallocated, 2->Z, 3->K
         @hw:
         @Rd: destination gp register or sp

*/
void System::decode_move_wide_imm(uint32_t inst) {
  uint8_t opc, hw, rd, shift;
  bool if_64bit;
  uint64_t imm;

  rd = bitutil::shift(inst, 0, 4);
  imm = bitutil::shift(inst, 5, 20);
  hw = bitutil::shift(inst, 21, 22);
  opc = bitutil::shift(inst, 29, 30);
  if_64bit = bitutil::bit(inst, 31);

  if (opc == 1 || ((if_64bit == 0) & (hw >= 2))) {
    unallocated();
  }

  shift = hw * 16;
  imm = imm << shift;
  LOG_CPU("MOV rd=%d, imm=%lu\n", rd, imm);

  switch (opc) {
  case 0: /* MOVN */
    imm = ~imm;
    [[fallthrough]];
  case 2: /* MOVZ */
    if (if_64bit) {
      cpu_.xregs[rd] = imm;
    } else {
      cpu_.xregs[rd] = bitutil::clear_upper32(imm);
    }
    break;
  case 3: /* MOVK */
    cpu_.xregs[rd] = cpu_.xregs[rd] | imm;
    break;
  default:
    assert(false);
  }
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
void System::decode_bitfield(uint32_t inst) {
  uint8_t rd, rn, imms, immr, opc, to, from, len;
  bool N, if_64bit;
  uint32_t imm;

  rd = bitutil::shift(inst, 0, 4);
  rn = bitutil::shift(inst, 5, 9);
  imms = bitutil::shift(inst, 10, 15);
  immr = bitutil::shift(inst, 16, 21);
  N = bitutil::bit(inst, 22);
  opc = bitutil::shift(inst, 29, 30);
  if_64bit = bitutil::bit(inst, 31);

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
  imm = bitutil::shift(cpu_.xregs[rn], from, from + len - 1) << to;

  switch (opc) {
  case 0:
    LOG_CPU("SBFM, to=%d, from=%d, len=%d\n", to, from, len);
    cpu_.xregs[rd] = 0;
    cpu_.xregs[rd] = signed_extend(imm, /*topbit=*/to + len - 1);
    break;
  case 1:
    LOG_CPU("BFM\n");
    cpu_.xregs[rd] = imm | (cpu_.xregs[rd] & ((1 << to) - 1)) |
                     (cpu_.xregs[rd] & (bitutil::mask(len) << to));
    break;
  case 2:
    LOG_CPU("UBFM\n");
    cpu_.xregs[rd] = imm;
    break;
  default:
    assert(false);
  }
}

void System::decode_extract(uint32_t inst) { LOG_CPU("%d\n", inst); }

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
                                 @opt: 000:no-allocate, 001:post-indexed,
   010:offset, 011:pre-indexed
                                 @L: if load
*/
void System::decode_ldst_register_pair(uint32_t inst) {
  bool if_vector, wback, postindex, if_32bit;
  uint8_t if_load, opc, opt, rn, rt, rt2, imm7;
  int16_t offset;
  uint64_t data1, data2;
  int64_t address;

  opc = bitutil::shift(inst, 30, 31);
  if_vector = bitutil::bit(inst, 26);
  opt = bitutil::shift(inst, 23, 25);
  if_load = bitutil::bit(inst, 22);
  imm7 = bitutil::shift(inst, 15, 21);
  rt2 = bitutil::shift(inst, 10, 14);
  rn = bitutil::shift(inst, 5, 9);
  rt = bitutil::shift(inst, 0, 4);

  if_32bit = 0;

  switch (opc) {
  case 0:
    if_32bit = 1;
    if (if_load) {
      LOG_CPU("LDP(32bit)");
    } else {
      LOG_CPU("STP(32bit)");
    }
    break;
  case 1:
    if (if_load) {
      LOG_CPU("LDPSW");
    } else {
      LOG_CPU("LDPSTGP");
    }
    break;
  case 2:
    if (if_load) {
      LOG_CPU("LDP(64bit)");
    } else {
      LOG_CPU("STP(64bit)");
    }
    break;
  case3:
    unallocated();
    break;
  default:
    unsupported();
  }

  offset = imm7;
  if (bitutil::bit(imm7, 6)) {
    offset = ~(imm7 - 1) & bitutil::mask(7);
    offset *= -1;
  }
  LOG_CPU("offset(before shift)=%d\n", offset);
  if (if_32bit) {
    offset *= 4;
  } else {
    offset *= 8;
  }
  LOG_CPU("offset(after shift)=%d", offset);

  switch (opt) {
  case 0:
    unsupported();
    break;
  case 1:
    wback = true;
    postindex = true;
    break;
  case 2:
    wback = false;
    postindex = false;
    break;
  case 3:
    wback = true;
    postindex = false;
    break;
  default:
    unallocated();
  }

  address = cpu_.xregs[rn];
  if (!postindex) {
    LOG_CPU("\tpre_indexed\n");
    address += offset;
  }

  if (if_load) {
    if (if_32bit) {
      cpu_.xregs[rt] = mem_.read(mem::MemAccess::Size32, address);
      cpu_.xregs[rt2] = mem_.read(mem::MemAccess::Size32, address + 4);
    } else {
      cpu_.xregs[rt] = mem_.read(mem::MemAccess::Size64, address);
      cpu_.xregs[rt2] = mem_.read(mem::MemAccess::Size64, address + 8);
    }
  } else {
    data1 = cpu_.xregs[rt];
    data2 = cpu_.xregs[rt2];
    if (if_32bit) {
      mem_.write(mem::MemAccess::Size32, address, data1);
      mem_.write(mem::MemAccess::Size32, address + 4, data2);
    } else {
      mem_.write(mem::MemAccess::Size64, address, data1);
      mem_.write(mem::MemAccess::Size64, address + 8, data2);
    }
  }

  if (wback) {
    if (postindex) {
      LOG_CPU("\tpost_indexed\n");
      address += offset;
    }
    LOG_CPU("\twrite back\n");
    cpu_.xregs[rn] = address;
  }
}

void System::decode_ldst_register(uint32_t inst) {
  uint8_t op;
  uint8_t op0 = bitutil::shift(inst, 28, 31);

  if (bitutil::bit(inst, 24)) {
    decode_ldst_reg_unsigned_imm(inst);
  } else {
    op = (bitutil::bit(inst, 21)) << 2 | bitutil::shift(inst, 10, 11);
    const decode_func decode_ldst_reg_tbl[] = {
        &System::decode_ldst_reg_immediate,
        &System::decode_ldst_reg_immediate,
        &System::decode_ldst_reg_unpriviledged,
        &System::decode_ldst_reg_immediate,
        &System::decode_ldst_atomic_memory_op,
        &System::decode_ldst_reg_pac,
        &System::decode_ldst_reg_reg_offset,
        &System::decode_ldst_reg_pac,
    };
    (this->*decode_ldst_reg_tbl[op])(inst);
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
void System::decode_ldst_reg_unsigned_imm(uint32_t inst) {
  bool vector;
  uint8_t size, opc, rn, rt;
  uint64_t imm12, offset;

  size = bitutil::shift(inst, 30, 31);
  vector = bitutil::bit(inst, 26);
  opc = bitutil::shift(inst, 22, 23);
  imm12 = bitutil::shift(inst, 10, 21);
  rn = bitutil::shift(inst, 5, 9);
  rt = bitutil::shift(inst, 0, 4);

  if (vector) {
    unsupported();
  } else {
    switch (opc) {
    case 0x0:
      LOG_CPU("STR, opc=0x%x, size=0x%x, V=%d, rt=%d, rn=%d\n", opc, size,
              vector, rt, rn);
      offset = imm12 << size;
      mem_.write((mem::MemAccess)size, cpu_.xregs[rn] + offset,
                 rt == 31 ? 0 : cpu_.xregs[rt]);
      break;
    case 0x1:
      LOG_CPU("LDR, opc=0x%x, size=0x%x, V=%d, rt=%d, rn=%d, inst=0x%x\n", opc,
              size, vector, rt, rn, inst);
      offset = imm12 << size;
      cpu_.xregs[rt] = mem_.read((mem::MemAccess)size, cpu_.xregs[rn] + offset);
      break;
    case 0x2:
      LOG_CPU("LDRS, opc=0x%x, size=0x%x, V=%d\n", opc, size, vector);
      break;
    case 0x3:
      if (size >= 0x2) {
        unallocated();
        return;
      }
      LOG_CPU("LDRS, opc=0x%x, size=0x%x, V=%d\n", opc, size, vector);
      break;
    }
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
         @idx: 00:unscaled immediate, 01:post-indexed, 11:pre-indexed
         @Rn: base register or stack pointer
         @Rt: register to be transfered

*/

void System::decode_ldst_reg_immediate(uint32_t inst) {
  bool vector;
  uint8_t size, opc, rn, rt, idx;
  uint64_t imm9, offset;
  bool post_indexed, writeback;

  size = bitutil::shift(inst, 30, 31);
  vector = bitutil::bit(inst, 26);
  opc = bitutil::shift(inst, 22, 23);
  imm9 = bitutil::shift(inst, 12, 20);
  idx = bitutil::shift(inst, 10, 11);
  rn = bitutil::shift(inst, 5, 9);
  rt = bitutil::shift(inst, 0, 4);

  switch (idx) {
  case 0:
  case 2:
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
  }

  if (vector) {
    unsupported();
  } else {

    if (post_indexed) {
      offset = 0;
    } else {
      offset = signed_extend(imm9, 9);
    }

    switch (opc) {
    case 0b00:
      /* STR */
      LOG_CPU("load_store: register: STR: size=%d, rt=%d, rn=%d, "
              "offset=%lu\n",
              size_tbl[size], rt, rn, offset);
      mem_.write((mem::MemAccess)size, cpu_.xregs[rn] + offset, cpu_.xregs[rt]);
      break;
    case 0b01:
      /* LDR (unsigned) */
      LOG_CPU("load_store: register: LDR(unsigned): size=%d, rt=%d, rn=%d, "
              "offset=%lu\n",
              size_tbl[size], rt, rn, offset);
      if (size == 3) {
        cpu_.xregs[rt] =
            mem_.read((mem::MemAccess)size, cpu_.xregs[rn] + offset);
      } else {
        cpu_.update_lower32(
            rt, mem_.read((mem::MemAccess)size, cpu_.xregs[rn] + offset));
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
      cpu_.xregs[rt] = signed_extend(
          mem_.read((mem::MemAccess)size, cpu_.xregs[rn] + offset),
          size_tbl[size]);
      break;
    case 0b11:
      /* LDR (signed 32bit) */
      if (size >= 2) {
        unallocated();
      }
      LOG_CPU("load_store: register: LDR(signed 32bit): size=%d, rt=%d, rn=%d, "
              "offset=%lu\n",
              size_tbl[size], rt, rn, offset);
      cpu_.update_lower32(rt,
                          signed_extend32(mem_.read((mem::MemAccess)size,
                                                    cpu_.xregs[rn] + offset),
                                          size_tbl[size]));
      break;
    }

    if (writeback) {
      LOG_CPU("\twriteback address\n");
      cpu_.xregs[rn] = cpu_.xregs[rn] + offset;
    }
  }
}

void System::decode_ldst_reg_unpriviledged(uint32_t inst) {
  LOG_CPU("load_store: ldst_reg_unpriviledged\n");
  LOG_CPU("%d\n", inst);
}
void System::decode_ldst_atomic_memory_op(uint32_t inst) {
  LOG_CPU("load_store: ldst_reg_atomic_memory_op\n");
  LOG_CPU("%d\n", inst);
}
void System::decode_ldst_reg_pac(uint32_t inst) {
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
void System::decode_ldst_reg_reg_offset(uint32_t inst) {
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
      offset =
          shift_and_extend(cpu_.xregs[rm], shift, scale, extendtype_tbl[opt]);
      mem_.write((mem::MemAccess)size, cpu_.xregs[rn] + offset, cpu_.xregs[rt]);
      LOG_CPU("load_store: register offset: store: rt %d [rn] %lu offset %lu\n",
              rt, cpu_.xregs[rn], offset);
      break;
    case 0b11: /* loads */
      if (size >= 0b10) {
        unallocated();
      }
      break;
    case 0b01: /* loadu */
    case 0b10: /* loads */
               // TODO: option = 0b011
      offset =
          shift_and_extend(cpu_.xregs[rm], shift, scale, extendtype_tbl[opt]);
      LOG_CPU("load_store: register offset: load: rt=%d, rn=%d, [rn]=0x%lx, "
              "rm=%d, "
              "[rm]=0x%lx, offset=%lu vaddr=0x%lx\n",
              rt, rn, cpu_.xregs[rn], rm, cpu_.xregs[rm], offset,
              cpu_.xregs[rn] + offset);
      cpu_.xregs[rt] = mem_.read((mem::MemAccess)size, cpu_.xregs[rn] + offset);
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
         @Rn: source gp regiter or sp
*/
void System::decode_addsub_shifted_reg(uint32_t inst) {
  uint8_t rd, rn, imm6;
  uint64_t rm, operand2;
  bool shift, setflag, if_sub, if_64bit;

  if_64bit = bitutil::bit(inst, 31);
  if_sub = bitutil::bit(inst, 30);
  setflag = bitutil::bit(inst, 29);
  shift = bitutil::shift(inst, 22, 23);
  rm = bitutil::shift(inst, 16, 20);
  imm6 = bitutil::shift(inst, 10, 15);
  rn = bitutil::shift(inst, 5, 9);
  rd = bitutil::shift(inst, 0, 4);

  operand2 = cpu_.xregs[rm] << imm6;

  if (if_sub) {
    LOG_CPU("SUB rd=0x%x, rm=0x%lx, rn=0x%d\n", rd, rm, rn);
  } else {
    LOG_CPU("ADD rd=0x%x, rm=0x%lx, rn=0x%d, xregs[rn]=0x%lx, op2=0x%lx\n", rd,
            rm, rn, cpu_.xregs[rn], operand2);
    cpu_.xregs[rd] = cpu_.xregs[rn] + operand2;
  }
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
void System::decode_logical_shifted_reg(uint32_t inst) {
  uint8_t opc, rd, rn, rm, imm6, shift;
  uint64_t op1, op2;
  bool setflag, if_64bit, if_not;

  if_64bit = bitutil::bit(inst, 31);
  opc = bitutil::shift(inst, 29, 30);
  shift = bitutil::shift(inst, 22, 23);
  if_not = bitutil::bit(inst, 21);
  rm = bitutil::shift(inst, 16, 20);
  imm6 = bitutil::shift(inst, 10, 15);
  rn = bitutil::shift(inst, 5, 9);
  rd = bitutil::shift(inst, 0, 4);

  op1 = rn == 31 ? 0 : cpu().xregs[rn];

  op2 = cpu().xregs[rm];
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

  switch (opc) {
  case 1:
    LOG_CPU("ORR rd[%d]:0x%lx, rn[%d]:0x%lx, rm[%d](shift:%d):0x%lx\n", rd,
            cpu().xregs[rd], rn, op1, rm, imm6, op2);
    cpu().xregs[rd] = op1 | op2;
    break;
  default:
    unsupported();
    break;
  }
}

/*
===========================================================
   Branches, Exception Generating and System instructions
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
static bool check_b_flag(uint8_t cond, core::cpu::CPSR &cpsr) {
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

void System::decode_conditional_branch_imm(uint32_t inst) {
  uint64_t offset;
  uint32_t imm19;
  uint8_t o1, o0, cond;

  o1 = bitutil::bit(inst, 24);
  imm19 = bitutil::shift(inst, 5, 23);
  o0 = bitutil::bit(inst, 4);
  cond = bitutil::shift(inst, 0, 3);

  if (o1 == 1) {
    unallocated();
  }

  if (o0 == 0) {
    if (check_b_flag(cond, cpu_.cpsr)) {
      offset = signed_extend(imm19 << 2, 20);
      LOG_CPU("B.cond: pc=0x%lx offset=0x%lx, cond=0x%x\n", cpu_.pc + offset,
              offset, cond);
      cpu_.set_pc(cpu_.pc + offset);
      return;
    }
  }
  cpu_.increment_pc();
}

/*
         Unconditional branch (reg)

          31     25  24  21 20  16  15   10 9    5  4    0
         +---------+-------------------------------------+
         | 1101011 |  opc  |  op2  |  op3  |  Rn  |  op4  |
         +---------+-------------------------------------+

         @op:
*/
void System::decode_unconditional_branch_reg(uint32_t inst) {
  uint8_t opc, op2, op3, Rn, op4, n;
  uint64_t target = cpu_.pc;

  opc = bitutil::shift(inst, 21, 24);
  op2 = bitutil::shift(inst, 16, 20);
  op3 = bitutil::shift(inst, 10, 15);
  Rn = bitutil::shift(inst, 5, 9);
  op4 = bitutil::shift(inst, 0, 4);

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
      target = cpu_.xregs[n];
      LOG_CPU("RET: target=xregs[%d](0x%lx)\n", n, target);
      break;
    default:
      unsupported();
    }
    break;
  default:
    unsupported();
  }
  cpu_.set_pc(target);
}

/*
         Unconditional branch (immediate)

           31   30  26 25                                  0
         +----+-------+------------------------------------+
         | op | 00101 |             imm26                  |
         +----+-------+------------------------------------+

         @op: 0->Branch, 1->Branch with Link

*/
void System::decode_unconditional_branch_imm(uint32_t inst) {
  uint64_t offset;
  uint32_t imm26;
  uint8_t op;

  op = bitutil::bit(inst, 31);
  imm26 = bitutil::shift(inst, 0, 25);

  offset = signed_extend(imm26 << 2, 27);

  switch (op) {
  case 0:
    LOG_CPU("B: pc=0x%lx offset=0x%lx, jumpto=0x%lx\n", cpu_.pc, offset,
            cpu_.pc + offset);
    break;
  case 1:
    cpu_.xregs[30] = cpu_.pc + 4;
    LOG_CPU("BL: pc=0x%lx, offset=0x%lx, jumpto=0x%lx, xregs[30]=0x%lx\n",
            cpu_.pc, offset, cpu_.pc + offset, cpu_.xregs[30]);
    break;
  }
  cpu_.set_pc(cpu_.pc + offset);
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
void System::decode_compare_and_branch_imm(uint32_t inst) {
  uint64_t offset;
  uint32_t imm19;
  uint8_t op, if_64bit, rt;
  bool if_jump = false;

  if_64bit = bitutil::bit(inst, 31);
  op = bitutil::bit(inst, 24);
  imm19 = bitutil::shift(inst, 5, 23);
  rt = bitutil::shift(inst, 0, 4);

  switch (op) {
  case 0: // CBZ
    if (if_64bit) {
      if_jump = cpu_.xregs[rt] == 0;
    } else {
      if_jump = (uint32_t)cpu_.xregs[rt] == 0;
    }
    break;
  case 1: // CBNZ
    if (if_64bit) {
      if_jump = cpu_.xregs[rt] != 0;
    } else {
      if_jump = (uint32_t)cpu_.xregs[rt] != 0;
    }
    break;
  }

  if (if_jump) {
    offset = signed_extend(imm19 << 2, 20);
    cpu_.set_pc(cpu_.pc - 4 + offset);
  }
}

} // namespace core
