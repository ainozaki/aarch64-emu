#include "cpu.h"

#include <algorithm>
#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <unistd.h>

#include "log.h"
#include "utils.h"

typedef __attribute__((mode(TI))) unsigned int uint128_t;
typedef __attribute__((mode(TI))) int int128_t;

void Cpu::init(uint64_t entry, uint64_t sp_base, uint64_t text_start,
               uint64_t text_size, uint64_t map_base) {
  pc = entry;
  sp = sp_base;
  printf("Init pc=0x%lx, sp=0x%lx\n", pc, sp);

  bus.init(text_start, text_size, map_base);
  mmu.init(&bus, &CurrentEL);
}

uint32_t Cpu::fetch() {
  // show_regs();
  // show_stack();
  return bus.load(mmu.mmu_translate(pc), MemAccessSize::Word);
}

uint64_t Cpu::load(uint64_t address, MemAccessSize size) {
  // printf("load 0x%lx\n", address);
  uint64_t paddr = mmu.mmu_translate(address);
  return bus.load(paddr, size);
}

void Cpu::store(uint64_t address, uint64_t value, MemAccessSize size) {
  uint64_t paddr = mmu.mmu_translate(address);
  bus.store(paddr, value, size);
}

void Cpu::decode_start(uint32_t inst) {
  uint8_t op1;
  op1 = util::shift(inst, 25, 28);

  /*
  printf("pc 0x%lx\n", pc);
  // printf("sp=0x%lx:\n", sp);
  // printf("0x%lx: \t", pc);
  printf("sp 0x%lx\n", sp);
  //printf("cpsr 0x%x\n", cpsr);
  printf("x0 0x%lx\n", xregs[0]);
  printf("x1 0x%lx\n", xregs[1]);
  printf("x2 0x%lx\n", xregs[2]);
  printf("x3 0x%lx\n", xregs[3]);
  printf("x19 0x%lx\n", xregs[19]);
  printf("x20 0x%lx\n", xregs[20]);
  printf("x29 0x%lx\n", xregs[29]);
  printf("x30 0x%lx\n", xregs[30]);
  if (mmu.if_mmu_enabled()) {
    bus.mem.debug_mem(mmu.mmu_translate(0xffffff8040016118));
  }
  else {
    bus.mem.debug_mem(0x40016118);
  }
  */
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

void Cpu::show_regs() {
  std::cout << "=================================================" << std::endl;
  printf("registers:\n");
  for (int i = 0; i < 32; i += 4) {
    printf("\tw%2d: 0x%16lx", i, xregs[i]);
    printf("\tw%2d: 0x%16lx", i + 1, xregs[i + 1]);
    printf("\tw%2d: 0x%16lx", i + 2, xregs[i + 2]);
    printf("\tw%2d: 0x%16lx\n", i + 3, xregs[i + 3]);
  }
  printf("\tpc: 0x%lx\n", pc);
  std::cout << "=================================================" << std::endl;
}
void Cpu::show_stack() {
  std::cout << "=================================================" << std::endl;
  printf("stack:\n");
  for (int i = 0; i < 20; i++) {
    printf("\tsp+0x%x: 0x%16lx\n", i * 8,
           load(sp + 8 * i, MemAccessSize::DWord));
  }
  std::cout << "=================================================" << std::endl;
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

[[maybe_unused]] const char *size_strtbl[] = {"b", "h", "w", "x"};

[[maybe_unused]] const char *shift_type_strtbl[] = {
    "LSL",
    "LSR",
    "ASR",
    "ROR",
};

static inline uint64_t bitmask64(uint8_t len) { return ~0ULL >> (64 - len); }

static inline uint64_t signed_extend(uint64_t val, uint8_t topbit) {
  return util::bit(val, topbit) ? (val | ~util::mask(topbit)) : val;
}
} // namespace

void Cpu::unsupported() {
  printf("unsupported inst at pc 0x%lx\n", pc);
  exit(1);
}

void Cpu::unallocated() {
  printf("unallocated inst\n");
  exit(1);
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
  uint8_t op0, op1, op2;
  uint16_t op3 /*, op4*/;

  op0 = util::shift(inst, 28, 31);
  op1 = util::bit(inst, 26);
  op2 = util::shift(inst, 23, 24);
  op3 = util::shift(inst, 16, 21);
  // op4 = util::shift(inst, 0, 11);

  switch (op0 & 3) {
  case 0b00:
    if (op1) {
      unsupported();
    } else {
      if (op2 == 1) {
        if (op3 >> 5) {
          LOG_CPU("compare and swap\n");
          unsupported();
          break;
        } else {
          decode_ldst_ordered(inst);
          break;
        }
      } else if (op2 == 0) {
        if (op3 >> 5) {
          if (op0 >> 4) {
            decode_ldst_exclusive(inst);
            break;
          } else {
            LOG_CPU("compare and swap pair\n");
            unsupported();
            break;
          }
        } else {
          decode_ldst_exclusive(inst);
          break;
        }
      } else {
        unallocated();
        break;
      }
    }
    break;
  case 0b01:
    switch (op0 >> 2) {
    case 3:
      LOG_CPU("load/store memory tags\n");
      unsupported();
      break;
    default:
      switch (util::shift(inst, 22, 24)) {
      case 0:
      case 1:
        decode_ldst_load_register_literal(inst);
        break;
      case 2:
      case 3:
        switch (util::shift(inst, 10, 11)) {
        case 0:
          LOG_CPU("LDAPR/STLR (unscaled immediate)\n");
          break;
        case 1:
          LOG_CPU("Memory Copy and Memory Set\n");
          break;
        default:
          assert(false);
        }
        break;
      default:
        assert(false);
      }
      break;
    }
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
  uint8_t op0, op1, op2;

  op0 = util::bit(inst, 30);
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
      LOG_CPU("conditional branch\n");
      unsupported();
      break;
    case 4:
      decode_conditional_select(inst);
      break;
    case 6:
      if (op0) {
        decode_data_processing_1source(inst);
      } else {
        decode_data_processing_2source(inst);
      }
      break;
    case 1:
    case 3:
    case 5:
    case 7:
      unallocated();
      break;
    default:
      decode_data_processing_3source(inst);
      break;
    }
    break;
  default:
    assert(false);
  }
  increment_pc();
}

void Cpu::decode_data_processing_float([[maybe_unused]] uint32_t inst) {
  LOG_CPU("data_processing_float %d\n", inst);
  increment_pc();
}

void Cpu::decode_branches(uint32_t inst) {
  uint8_t op0 = util::shift(inst, 29, 31);
  switch (op0) {
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
      decode_test_and_branch_imm(inst);
    }
    break;
  case 6:
    switch (util::shift(inst, 24, 25)) {
    case 0:
      decode_exception_generation(inst);
      break;
    case 1:
      if (util::bit(inst, 20)) {
        decode_system_register_move(inst);
      } else if (util::bit(inst, 19)) {
        decode_system_instructions(inst);
      } else if (util::shift(inst, 12, 17) == 0b110001) {
        LOG_CPU("System instructions with register argument\n");
        unsupported();
      } else if (util::shift(inst, 12, 17) == 0b110010) {
        LOG_CPU("Hints\n");
        unsupported();
      } else if (util::shift(inst, 12, 17) == 0b110011) {
        decode_barriers(inst);
      } else if (util::shift(inst, 12, 15) == 0b0100) {
        decode_pstate(inst);
      } else {
        LOG_CPU("systems\n");
        unallocated();
      }
      increment_pc();
      break;
    case 2:
    case 3:
      decode_unconditional_branch_reg(inst);
      break;
    default:
      assert(false);
    }
    break;
  default:
    assert(false);
    break;
  }
}

void Cpu::decode_sme_encodings([[maybe_unused]] uint32_t inst) {
  LOG_CPU("sme_encodings 0x%x\n", inst);
  //mmu.mmu_debug(pc);
  increment_pc();
  exit(1);
}

void Cpu::decode_unallocated([[maybe_unused]] uint32_t inst) {
  LOG_CPU("unallocated %x\n", inst);
  increment_pc();
  exit(1);
}

void Cpu::decode_sve_encodings([[maybe_unused]] uint32_t inst) {
  LOG_CPU("sve_encodings %x\n", inst);
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
  uint64_t result = unsigned_sum & 0xffffffffffffffff;
  cpsr.N = util::bit64(result, 63);
  cpsr.Z = result == 0;
  if (carry_in) {
    cpsr.C = (int64_t)x >= (int64_t)~y;
  } else {
    cpsr.C = (int64_t)x > (int64_t)y;
  }
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
  [[maybe_unused]] const char *op;

  rd = util::shift(inst, 0, 4);
  rn = util::shift(inst, 5, 9);
  imm = util::shift(inst, 10, 21);
  if_shift = util::bit(inst, 22);
  if_setflag = util::bit(inst, 29);
  if_sub = util::bit(inst, 30);
  if_64bit = util::bit(inst, 31);
  op = if_sub ? "sub" : "add";

  op1 = (rn == 31) ? sp : xregs[rn];
  if (if_shift) {
    imm <<= 12;
  }

  if (if_setflag) {
    result = add_imm_s(op1, imm, if_sub, cpsr, if_64bit);
    LOG_CPU("%ss x%d, x%d, #0x%lx, LSL %d\n", op, rd, rn, imm, if_shift * 12);
  } else {
    result = add_imm(op1, imm, if_sub);
    LOG_CPU("%s x%d, x%d(=0x%lx), #0x%lx, LSL %d\n", op, rd, rn, xregs[rn], imm,
            if_shift * 12);
  }
  if (rd == 31) {
    if (if_setflag) {
      // ADDS/SUBS xzr
      return;
    } else {
      // ADD/SUB sp
      sp = if_64bit ? result : (result & util::mask(32));
    }
  } else {
    xregs[rd] = if_64bit ? result : (result & util::mask(32));
  }
}

void Cpu::decode_add_sub_imm_with_tags([[maybe_unused]] uint32_t inst) {
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

void Cpu::decode_logical_imm(uint32_t inst) {
  uint8_t rd, rn, imms, immr, opc;
  bool N, if_64bit;
  uint64_t imm;
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

  uint64_t xrn = rn == 31 ? 0 : xregs[rn];
  switch (opc) {
  case 0b00:
    LOG_CPU("and x%d, x%d(=0x%lx), #0x%lx\n", rd, rn, xrn, imm);
    result = xrn & imm; /* AND */
    break;
  case 0b01:
    LOG_CPU("orr x%d, x%d(=0x%lx), #%lx\n", rd, rn, xrn, imm);
    result = xrn | imm; /* ORR */
    break;
  case 0b10:
    LOG_CPU("eor x%d, x%d(=0x%lx), #%lx\n", rd, rn, xrn, imm);
    result = xrn ^ imm; /* EOR */
    break;
  case 0b11:
    LOG_CPU("ands x%d, x%d(=0x%lx), #%lx\n", rd, rn, xrn, imm);
    result = xrn & imm; /* ANDS */
    if (if_64bit) {
      cpsr.N = (int64_t)result < 0;
    } else {
      cpsr.N = (int32_t)result < 0;
    }
    cpsr.Z = result == 0;
    cpsr.C = 0;
    cpsr.V = 0;
    if (rd == 31){return;}
    break;
  default:
    assert(false);
  }

  if (rd == 31){
    sp = if_64bit ? result : (result & util::mask(32));
  }else {
    xregs[rd] = if_64bit ? result : (result & util::mask(32));
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
    xregs[rd] = if_64bit ? imm : imm & util::mask(32);
    break;
  case 3: /* MOVK */
    xregs[rd] = (xregs[rd] & ~util::mask(shift + 16)) | imm |
                (xregs[rd] & util::mask(shift));
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
    LOG_CPU("ubfm x%d, x%d, #0x%x, #0x%x\n", rd, rn, immr, imms);
    xregs[rd] = imm;
    break;
  default:
    assert(false);
  }
}

void Cpu::decode_extract([[maybe_unused]] uint32_t inst) {
  LOG_CPU("%d\n", inst);
}

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
  [[maybe_unused]] const char *op;

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
    LOG_CPU("%s x%d, x%d, [x%d], #%ld ", op, rt, rt2, rn, (int64_t)offset);
    break;
  case 2:
    wback = false;
    postindex = false;
    LOG_CPU("%s x%d, x%d, [x%d, #%ld] ", op, rt, rt2, rn, (int64_t)offset);
    break;
  case 3:
    wback = true;
    postindex = false;
    LOG_CPU("%s x%d, x%d, [x%d(=0x%lx), #%ld]! ", op, rt, rt2, rn, xregs[rn],
            (int64_t)offset);
    break;
  default:
    unallocated();
  }

  address = (rn == 31) ? sp : xregs[rn];
  if (!postindex) {
    address += (int64_t)offset;
  }
  LOG_CPU(",address=0x%lx\n", address);

  uint64_t value1, value2;
  if (if_load) {
    if (if_32bit) {
      value1 = load(address, MemAccessSize::Word);
      value2 = load(address + 4, MemAccessSize::Word);
      xregs[rt] = util::set_lower(xregs[rt], value1, MemAccessSize::Word);
      xregs[rt2] = util::set_lower(xregs[rt2], value2, MemAccessSize::Word);
    } else {
      xregs[rt] = load(address, MemAccessSize::DWord);
      xregs[rt2] = load(address + 8, MemAccessSize::DWord);
    }
  } else {
    data1 = xregs[rt];
    data2 = xregs[rt2];
    if (if_32bit) {
      store(address, data1, MemAccessSize::Word);
      store(address + 4, data2, MemAccessSize::Word);
    } else {
      store(address, data1, MemAccessSize::DWord);
      store(address + 8, data2, MemAccessSize::DWord);
    }
  }

  if (wback) {
    if (postindex) {
      address += offset;
    }
    if (rn == 31) {
      sp = address;
    } else {
      xregs[rn] = address;
    }
  }
}

void Cpu::decode_ldst_register(uint32_t inst) {
  uint8_t op2, op;
  // op0 = util::shift(inst, 28, 31);
  op2 = util::shift(inst, 23, 24);

  assert(op0 % 4 == 3);
  if (op2 >= 2) {
    decode_ldst_reg_immediate(inst);
  } else {
    op = (util::bit(inst, 21)) << 2 | util::shift(inst, 10, 11);
    const decode_func decode_ldst_reg_tbl[] = {
        &Cpu::decode_ldst_reg_unscaled_immediate,
        &Cpu::decode_ldst_reg_immediate,
        &Cpu::decode_ldst_reg_unpriviledged,
        &Cpu::decode_ldst_reg_immediate,
        &Cpu::decode_ldst_atomic_memory_op,
        &Cpu::decode_ldst_reg_pac,
        &Cpu::decode_ldst_reg_reg_offset,
        &Cpu::decode_ldst_reg_pac,
    };
    (this->*decode_ldst_reg_tbl[op])(inst);
  }
  return;
}

void Cpu::decode_ldst_reg_unscaled_immediate([[maybe_unused]] uint32_t inst) {
  LOG_CPU("ldst_reg_scucaled_imm %x\n", inst);
  unsupported();
}

/*
          TODO: merge with immediate post/pre indexed
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
  uint64_t imm12, offset, addr;

  size = util::shift(inst, 30, 31);
  vector = util::bit(inst, 26);
  opc = util::shift(inst, 22, 23);
  imm12 = util::shift(inst, 10, 21);
  rn = util::shift(inst, 5, 9);
  rt = util::shift(inst, 0, 4);

  if (vector) {
    LOG_CPU("ldst_reg_unsigned_imm\n");
    unsupported();
    return;
  }

  uint64_t value;
  switch (size) {
  case 0:
  case 1:
    LOG_CPU("ldr/str 8/16\n");
    unsupported();
    return;
  case 2:
    offset = imm12 << size;
    addr = (rn == 31) ? sp + offset : xregs[rn] + offset;
    switch (opc) {
    case 0:
      store(addr, xregs[rt], MemAccessSize::Word);
      LOG_CPU("str x%d(=0x%lx), [x%d, #%ld]\n", rt, xregs[rt], rn, offset);
      break;
    case 1:
      value = load(addr, MemAccessSize::Word);
      xregs[rt] = util::set_lower(xregs[rt], value, MemAccessSize::Word);
      LOG_CPU("ldr x%d(=0x%lx), [x%d, #%ld]\n", rt, xregs[rt], rn, offset);
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
    addr = (rn == 31) ? sp + offset : xregs[rn] + offset;
    switch (opc) {
    case 0:
      store(addr, xregs[rt], MemAccessSize::DWord);
      LOG_CPU("str x%d(=0x%lx), [x%d(=0x%lx), #%ld]\n", rt, xregs[rt], rn,
              xregs[rn], offset);
      break;
    case 1:
      xregs[rt] = load(addr, MemAccessSize::DWord);
      LOG_CPU("ldr x%d(=0x%lx), [x%d, #%ld]\n", rt, xregs[rt], rn, offset);
      break;
    case 2:
      LOG_CPU("prfm\n");
      unsupported();
      return;
    }
    break;
  default:
    assert(false);
  }
}

/*
          Ok
         Load/store register (immediate pre-indexed)
         Load/store register (immediate post-indexed)

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

void Cpu::decode_ldst_reg_immediate(uint32_t inst) {
  bool vector;
  uint8_t size, opc, rn, rt, idx;
  uint64_t imm9, imm12, offset, address, value;
  bool post_indexed, writeback;

  size = util::shift(inst, 30, 31);
  vector = util::bit(inst, 26);
  opc = util::shift(inst, 22, 23);
  imm9 = util::shift(inst, 12, 20);
  imm12 = util::shift(inst, 10, 21);
  idx = util::shift(inst, 10, 11);
  rn = util::shift(inst, 5, 9);
  rt = util::shift(inst, 0, 4);

  if (util::bit(inst, 24)) {
    // unsined
    offset = imm12 << size;
    post_indexed = false;
    writeback = false;
  } else {
    offset = util::SIGN_EXTEND(imm9, 9);
    switch (idx) {
    case 1:
      // post indexed
      post_indexed = true;
      writeback = true;
      break;
    case 3:
      // pre indexed
      post_indexed = false;
      writeback = true;
    }
  }

  if (vector) {
    unsupported();
    return;
  }

  address = (rn == 31) ? sp : xregs[rn];

  if (!post_indexed) {
    address += offset;
  }

  switch (opc) {
  case 0b00:
    /* STR */
    LOG_CPU("str%s ", size_strtbl[size]);
    store(address, xregs[rt], memsz_tbl[size]);
    break;
  case 0b01:
    /* LDR (unsigned) */
    LOG_CPU("ldr%s ", size_strtbl[size]);
    value = load(address, memsz_tbl[size]);
    xregs[rt] = util::zero_extend(value, 8 * std::pow(2, size));
    break;
  case 0b10:
    /* LDR (signed 64bit) */
    if (size == 3) {
      unsupported();
      break;
    }
    LOG_CPU("ldrs%s(64bit) ", size_strtbl[size]);
    switch (size) {
    case 0:
    case 1:
    case 2:
      value = load(address, memsz_tbl[size]);
      xregs[rt] = util::SIGN_EXTEND(value, 8 * std::pow(2, size));
      break;
    case 3:
      unallocated();
      break;
    }
    break;
  case 0b11:
    /* LDR (signed 32bit) */
    if (size >= 2) {
      unallocated();
    }
    LOG_CPU("ldrs%s(32bit) ", size_strtbl[size]);
    switch (size) {
    case 0:
    case 1:
      value =
          util::SIGN_EXTEND(load(address, memsz_tbl[size]), 8 * pow(2, size));
      xregs[rt] = util::set_lower(xregs[rt], value, MemAccessSize::Word);
      break;
    case 2:
    case 3:
      unallocated();
      break;
    }
    break;
  }
  if (writeback) {
    xregs[rn] = xregs[rn] + offset;
    if (post_indexed) {
      LOG_CPU("x%d(=0x%lx), [x%d], #0x%lx, address=0x%lx\n", rt, xregs[rt], rn,
              offset, address);
    } else {
      LOG_CPU("x%d(=0x%lx), [x%d, #0x%lx]!, address=0x%lx\n", rt, xregs[rt], rn,
              offset, address);
    }
  } else {
    LOG_CPU("x%d(=0x%lx), [x%d, #0x%lx], address=0x%lx\n", rt, xregs[rt], rn,
            offset, address);
  }
}

void Cpu::decode_ldst_reg_unpriviledged([[maybe_unused]] uint32_t inst) {
  LOG_CPU("load_store: ldst_reg_unpriviledged 0x%x\n", inst);
  unsupported();
}
void Cpu::decode_ldst_atomic_memory_op([[maybe_unused]] uint32_t inst) {
  LOG_CPU("load_store: ldst_reg_atomic_memory_op 0x%x\n", inst);
  unsupported();
}
void Cpu::decode_ldst_reg_pac([[maybe_unused]] uint32_t inst) {
  LOG_CPU("load_store: ldst_reg_pca 0x%x\n", inst);
  unsupported();
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
    len = 7;
    break;
  case ExtendType::SXTH:
    if_unsigned = false;
    len = 15;
    break;
  case ExtendType::SXTW:
    if_unsigned = false;
    len = 31;
    break;
  case ExtendType::SXTX:
    if_unsigned = false;
    len = 63;
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
  uint64_t offset, address;

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
      address = (rn == 31) ? sp + offset : xregs[rn] + offset;
      store(address, xregs[rt], memsz_tbl[size]);
      LOG_CPU("str x%d, [x%d, x%d {#%d}] (=0x%lx)\n", rt, rn, rm, shift * 3,
              xregs[rn] + offset);
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
      address = (rn == 31) ? sp + offset : xregs[rn] + offset;
      LOG_CPU("ldr x%d, [x%d, x%d {#%d}] (=0x%lx)\n", rt, rn, rm, shift * 3,
              xregs[rn] + offset);
      xregs[rt] = load(address, MemAccessSize::DWord) & util::mask(8 * std::pow(2, size));
      break;
    }
  }
}

/*
         Load/store load register (literal)

         31   30 29 27 26  25 24 23                      5 4   0
         +------+-----+---+----+--------------------------+----+
         | opc  | 011 | V | 00 |         imm19            | Rt |
         +------+-----+---+----+--------------------------+----+

         @opc: 00->LDR(32bit), 01->LDR(64bit), 10->LDRSW, 11->PRFM
         @V: simd
*/

void Cpu::decode_ldst_load_register_literal(uint32_t inst) {
  bool if_vector;
  uint8_t opc, rt;
  uint64_t imm19, offset, address, data;

  opc = util::shift(inst, 30, 31);
  if_vector = util::bit(inst, 26);
  imm19 = util::shift(inst, 5, 23);
  rt = util::shift(inst, 0, 4);

  if ((opc == 3) && if_vector) {
    unallocated();
    return;
  }

  offset = util::SIGN_EXTEND(imm19 << 2, 21);
  address = pc + offset;
  data = load(address, MemAccessSize::DWord);

  switch (opc) {
  case 0:
    xregs[rt] = util::set_lower32(inst, data);
    break;
  case 1:
    xregs[rt] = data;
    break;
  case 2:
    xregs[rt] = util::SIGN_EXTEND(data, 32);
    break;
  default:
    LOG_CPU("ldst_load_register_literal, opc=3\n");
    unsupported();
    return;
  }

  LOG_CPU("ldr x%d(=0x%lx), 0x%lx\n", rt, xregs[rt], address);
}

/*
         Load/store exclusive pair

          31   30  29    23  22   21  20   16  15  14      10 9    5 4   0
         +---+----+---------+---+----+-------+----+----------+------+-----+
         | 1 | sz | 0010000 | L | 1 |   Rs   | o0 |   Rt2    |  Rn  |  Rt |
         +---+----+---------+---+----+-------+----+----------+------+-----+

         Load/store exclusive register

          31 30  29    23  22   21  20   16  15  14      10 9    5 4   0
         +------+---------+---+----+-------+----+----------+------+-----+
         | size | 0010000 | L | 0 |   Rs   | o0 |   Rt2    |  Rn  |  Rt |
         +------+---------+---+----+-------+----+----------+------+-----+

         @opc: 00->LDR(32bit), 01->LDR(64bit), 10->LDRSW, 11->PRFM
         @V: simd
*/

void Cpu::decode_ldst_exclusive(uint32_t inst) {
  bool if_pair, if_load, if_acquire;
  uint8_t size, rs, /*rt2,*/ rt, rn;
  uint64_t data, address;

  if_pair = util::bit(inst, 21);
  size = util::shift(inst, 30, 31);
  if_load = util::bit(inst, 22);
  rs = util::shift(inst, 16, 20);
  if_acquire = util::bit(inst, 15);
  // rt2 = util::shift(inst, 10, 14);
  rn = util::shift(inst, 5, 9);
  rt = util::shift(inst, 0, 4);

  address = (rn == 31) ? sp : xregs[rn];

  if (if_pair) {
    if (if_load) {
      LOG_CPU("LDXP\n");
    } else {
      LOG_CPU("STXP\n");
    }
  } else {
    if (if_load) {
      if (if_acquire) {
        LOG_CPU("ldaxr\n");
        unsupported();
      } else {
        data = load(address, memsz_tbl[size]);
        xregs[rt] = (size == 11) ? util::zero_extend(data, 64)
                                 : util::zero_extend(data, 32);
        LOG_CPU("ldxr x%d(=0x%lx), x%d(=0x%lx)\n", rt, xregs[rt], rn, address);
      }
    } else {
      if (if_acquire) {
        LOG_CPU("stlxr\n");
        unsupported();
      } else {
        store(address, xregs[rt], memsz_tbl[size]);
        xregs[rs] = 0;
        LOG_CPU("stxr x%d, x%d(=0x%lx), x%d(=0x%lx)\n", rs, rt, xregs[rt], rn,
                xregs[rn]);
      }
    }
  }
}

/*
         Load/store ordered

          31 30  29    23  22   21  20   16  15  14      10 9    5 4   0
         +------+---------+---+----+-------+----+----------+------+-----+
         | size | 0010001 | L | 0 |   Rs   | o0 |   Rt2    |  Rn  |  Rt |
         +------+---------+---+----+-------+----+----------+------+-----+

         @L: 0->store, 1->load
         @o0: 0->FEAT_LOR
*/

void Cpu::decode_ldst_ordered(uint32_t inst) {
  bool if_load, if_lor;
  uint8_t size, rt, rn;
  uint64_t data, address;

  size = util::shift(inst, 30, 31);
  if_load = util::bit(inst, 22);
  if_lor = !util::bit(inst, 15);
  rn = util::shift(inst, 5, 9);
  rt = util::shift(inst, 0, 4);

  address = rn == 31 ? sp : xregs[rn];

  if (if_lor) {
    LOG_CPU("decode_ldst_ordered lor\n");
    unsupported();
    return;
  }

  if (if_load) {
    data = load(address, memsz_tbl[size]);
    xregs[rt] = (size == 11) ? util::zero_extend(data, 64)
                             : util::zero_extend(data, 32);
    LOG_CPU("ldar x%d(=0x%lx), x%d, address=0x%lx\n", rt, xregs[rt], rn,
            address);
  } else {
    LOG_CPU("stlr x%d(=0x%lx), x%d, address=0x%lx\n", rt, xregs[rt], rn,
            address);
    store(address, xregs[rt], memsz_tbl[size]);
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
  [[maybe_unused]] const char *op;

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
    LOG_CPU("%ss x%d, x%d(=0x%lx), x%d(=0x%lx), %s #%d\n", op, rd, rn,
            xregs[rn], rm, xregs[rm], shift_type_strtbl[shift_type],
            shift_amount);
  } else {
    result = add_imm(op1, op2, if_sub);
    LOG_CPU("%s x%d, x%d, x%d, %s #%d\n", op, rd, rn, rm,
            shift_type_strtbl[shift_type], shift_amount);
  }

  if (rd == 31) {
    return;
  }
  xregs[rd] = if_64bit ? result : util::mask(32) & result;
}

/*
        ok
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
  bool if_64bit, if_op2_64bit, if_sub, if_setflag;
  [[maybe_unused]] const char *op;

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
  if_op2_64bit = (opt % 4 == 3);

  if (opt || (shift_amount > 4)) {
    unallocated();
    return;
  }

  op1 = xregs[rn];
  op2 = if_op2_64bit ? xregs[rm] : util::clear_upper32(xregs[rm]);
  op2 = ExtendValue(op2, extend_type, shift_amount);
  result = if_setflag ? add_imm_s(op1, op2, if_sub, cpsr, if_64bit)
                      : add_imm(op1, op2, if_sub);
  if (rd != 31) {
    xregs[rd] = if_64bit ? result : util::set_lower32(xregs[rd], result);
  }
  LOG_CPU("%s x%d(=0x%lx), x%d(=0x%lx), x%d(=0x%lx), #%d\n", op, rd, xregs[rd],
          rn, xregs[rn], rm, xregs[rm], shift_amount);
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
    if ((shift == 0) && (imm6 == 0) && (rn == 31)){
      xregs[rd] = op2;
      LOG_CPU("mov x%d, x%d(0x%lx)\n", rd, rm, op2);
      break;
    }
    result = op1 | op2;
    xregs[rd] = if_64bit
                    ? result
                    : (~util::mask(32) & xregs[rd]) | (util::mask(32) & result);
    LOG_CPU("orr x%d, x%d(0x%lx) | x%d(0x%lx)\n", rd, rn, op1, rm, op2);
    break;
  default:
    LOG_CPU("decode_logical_shifted_reg\n");
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
  bool /*if_64bit, */ s;

  // if_64bit = util::bit(inst, 31);
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
      if (check_b_flag(cond)){
        result = xregs[rn];
      }else {
        result = ~xregs[rm] + 1;
      }
      xregs[rd] = result;
      LOG_CPU("csneg x%d(=0x%lx), x%d(=0x%lx), x%d(=0x%lx), cond=%d\n", rd, xregs[rd], rn, xregs[rn], rm, xregs[rm], cond);
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
         Data Processing (1 source)
           31   30  29   28   21  20   16 15    10  9   5 4  0
         +----+---+---+----------+-------+---------+-----+-----+
         | sf | 1 | S | 11010110 |  op2  |  opcode |  Rn |  Rd |
         +----+---+---+----------+-------+---------+-----+-----+

*/
void Cpu::decode_data_processing_1source([[maybe_unused]] uint32_t inst) {
  /*
   uint64_t result;
   uint8_t sf, s, op2, opcode, rn, rd;

   sf = util::bit(inst, 31);
   s = util::bit(inst, 29);
   op2 = util::shift(inst, 16, 20);
   opcode = util::shift(inst, 10, 15);
   rn = util::shift(inst, 5, 9);
   rd = util::shift(inst, 0, 4);
 */
  LOG_CPU("data processing 1 source, opcode = 0x%lx\n",
          util::shift(inst, 10, 15));
  unsupported();
}

/*
         Data Processing (2 source)
           31   30  29   28   21  20   16 15    10  9   5 4  0
         +----+---+---+----------+-------+---------+-----+-----+
         | sf | 0 | S | 11010110 |  rm   |  opcode |  Rn |  Rd |
         +----+---+---+----------+-------+---------+-----+-----+

*/
void Cpu::decode_data_processing_2source(uint32_t inst) {
  uint8_t sf, /*s, */ rm, opcode, rn, rd, datasize;
  uint64_t result;

  sf = util::bit(inst, 31);
  // s = util::bit(inst, 29);
  rm = util::shift(inst, 16, 20);
  opcode = util::shift(inst, 10, 15);
  rn = util::shift(inst, 5, 9);
  rd = util::shift(inst, 0, 4);

  datasize = sf ? 64 : 32;

  switch (opcode) {
  case 0b000010:
    if (rm == 0){
      xregs[rd] = 0;
    }else {
      result = xregs[rn] / xregs[rm];
      xregs[rd] = sf ? result: (result & util::mask(32));
    }
    LOG_CPU("udiv x%d(=0x%lx), x%d(=0x%lx), x%d(=0x%lx)\n", rd, xregs[rd], rn, xregs[rn], rm, xregs[rm]);
    break;
  case 0b000011:
    if (rm == 0){
      xregs[rd] = 0;
    }else {
      result = xregs[rn] / xregs[rm];
      xregs[rd] = sf ? result: signed_extend((result & util::mask(32)), 31);
    }
    LOG_CPU("sdiv x%d(=0x%lx), x%d(=0x%lx), x%d(=0x%lx)\n", rd, xregs[rd], rn, xregs[rn], rm, xregs[rm]);
    break;
  case 0b001000:
    LOG_CPU("lslv x%d, x%d, x%d\n", rd, rn, rm);
    unsupported();
    break;
  case 0b001001:
    result = xregs[rn] >> (xregs[rm] % datasize);
    xregs[rd] = result;
    LOG_CPU("lsrv x%d(=0x%lx), x%d(=0x%lx), x%d(=0x%lx)\n", rd, xregs[rd], rn,
            xregs[rn], rm, xregs[rm]);
    break;
  default:
    LOG_CPU("data processing 2 source\n");
    unsupported();
  }
}

/*
         Data Processing 3 source
           31  30  29  28  24  23 21  20  16  15  14  10 9   5 4   0
         +----+------+-------+------+-------+----+------+-----+-----+
         | sf | op54 | 11011 | op31 |   Rm  | o0 |  Ra  |  Rn |  Rd |
         +----+------+-------+------+-------+----+------+-----+-----+

         @sf: 0->32bit, 1->64bit
         @op: 00000*->M, 00001*->SM, 000100->SMULH, 00101*->UM, 001100->UMULH
*/
void Cpu::decode_data_processing_3source(uint32_t inst) {
  uint64_t result, operand1, operand2, operand3;
  uint8_t sf, op54, op31, o0, rm, ra, rn, rd, op;

  sf = util::bit(inst, 31);
  op54 = util::shift(inst, 29, 30);
  op31 = util::shift(inst, 21, 23);
  rm = util::shift(inst, 16, 20);
  o0 = util::bit(inst, 15);
  ra = util::shift(inst, 10, 14);
  rn = util::shift(inst, 5, 9);
  rd = util::shift(inst, 0, 4);
  op = (op54 << 4) | (op31 << 1) | o0;

  operand1 = xregs[rn];
  operand2 = xregs[rm];
  operand3 = xregs[ra];

  switch (op) {
  case 0:
    result = operand3 + operand1 * operand2;
    LOG_CPU("madd x%d(=0x%lx), x%d(=0x%lx), x%d(=0x%lx), x%d(=0x%lx)\n", rd,
            result, rn, operand1, rm, operand2, ra, operand3);
    break;
  case 1:
    LOG_CPU("msub\n");
    break;
  case 0b000010:
    LOG_CPU("smaddl\n");
    break;
  case 0b000011:
    LOG_CPU("smsubl\n");
    break;
  case 0b000100:
    LOG_CPU("smulh\n");
    break;
  case 0b001010:
    LOG_CPU("umaddl\n");
    break;
  case 0b001011:
    LOG_CPU("umsubl\n");
    break;
  case 0b001100:
    LOG_CPU("umulh\n");
    break;
  default:
    unallocated();
    return;
  }

  xregs[rd] = sf ? result : result & util::mask(32);
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
  switch (o0) {
  case 0:
    if (check_b_flag(cond)) {
      offset = signed_extend(imm19 << 2, 20);
      set_pc(pc + offset);
      LOG_CPU("B.cond: pc=0x%lx offset=0x%lx, cond=0x%x\n", pc + offset, offset,
              cond);
      return;
    } else {
      increment_pc();
      LOG_CPU("B.cond(not match): cond=0x%x\n", cond);
      return;
    }
    break;
  case 1:
    LOG_CPU("BC.cond");
    unsupported();
    increment_pc();
    break;
  default:
    assert(false);
  }
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
  [[maybe_unused]] uint64_t imm16;

  opc = util::shift(inst, 21, 23);
  imm16 = util::shift(inst, 5, 20);
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
      write(xregs[0],
            (void *)(bus.mem.map_base_ + xregs[1] - bus.mem.text_start_),
            xregs[2]);
      break;
    case 2:
      LOG_CPU("HVC 0x%lx\n", imm16);
      xregs[0] = (uint64_t)-2;
      set_pc(xregs[30]);
      return;
    case 3:
      LOG_CPU("SMC\n");
      break;
    default:
      unallocated();
    }
    break;
  default:
    LOG_CPU("exception_generation\n");
    unsupported();
    break;
  }
  increment_pc();
}

/*
         PASTATE

          31           19 18 16   15 12  11 8  7  5  4   0
         +---------------+------+------+------+-----+------+
         | 1101010100000 | op1  | 0100 |  CRm | op2 |  Rt  |
         +---------------+------+------+------+-----+------+

         @op2: 000->CFINV, 001->XAFLAG, 010->AXFLAG, other->MSR
*/
void Cpu::decode_pstate(uint32_t inst) {
  uint8_t CRm, op1, op2, rt;

  op1 = util::shift(inst, 16, 18);
  CRm = util::shift(inst, 8, 11);
  op2 = util::shift(inst, 5, 7);
  rt = util::shift(inst, 0, 4);

  if (rt != 31) {
    unallocated();
  }

  switch (op2) {
  case 0:
    LOG_CPU("cfinv\n");
    unsupported();
    break;
  case 1:
    LOG_CPU("xaflag\n");
    unsupported();
    break;
  case 2:
    LOG_CPU("axflag\n");
    unsupported();
    break;
  default:
    LOG_CPU("msr immediate\n");
    switch (op1) {
    case 3:
      switch (op2) {
      case 6:
        daif = daif | (CRm << 6);
        LOG_CPU("msr daifset(=0x%lx), 0x%x\n", daif, CRm);
        break;
      default:
        unsupported();
      }
      break;
    default:
      unsupported();
    }
    break;
  }
}

/*
         Barriers

          31                  12 11  8   7  5  4    0
         +----------------------+-------+----+-----+
         | 11010101000000110011 |  CRm  | op2|  Rt |
         +----------------------+-------+----+-----+

         @L:0->MSR, 1->MRS
*/
void Cpu::decode_barriers(uint32_t inst) {
  [[maybe_unused]] uint8_t CRm;
  uint8_t op2, rt;

  CRm = util::shift(inst, 8, 11);
  op2 = util::shift(inst, 5, 7);
  rt = util::shift(inst, 0, 4);

  switch (op2) {
  case 0b001:
    if (rt != 0b11111 || util::shift(inst, 8, 9) != 0b10) {
      unallocated();
      return;
    }
    /* Data Synchronization Barrier */
    LOG_CPU("dsb memory nXS barrier\n");
    break;
  case 0b101:
    /* Data Memory Barrier */
    LOG_CPU("dmb type = 0x%x\n", CRm);
    break;
  case 0b100:
    /* Data Synchronization Barrier */
    LOG_CPU("dsb memory barrier type = 0x%x\n", CRm);
    break;
  case 0b110:
    /* Instruction Synchronization Barrier */
    LOG_CPU("isb\n");
    break;
  default:
    unsupported();
  }
}

/*
         System register move

          31 29 28 26 25  22 21  20   19  18 16 15  12 11  8  7  5  4    0
         +-----+-----+------+---+---+----+-----+-----+-------+----+-----+
         | 110 | 101 | 0100 | L | 1 | o0 | o1 |  CRn |  CRm  | op2|  Rt |
         +-----+-----+------+---+---+----+-----+-----+-------+----+-----+

         @L:0->MSR, 1->MRS
*/
void Cpu::decode_system_register_move(uint32_t inst) {
  uint8_t o0, o1, CRn, CRm, op2, rt;
  bool if_get;

  if_get = util::bit(inst, 21);
  o0 = util::bit(inst, 19) + 2;
  o1 = util::shift(inst, 16, 18);
  CRn = util::shift(inst, 12, 15);
  CRm = util::shift(inst, 8, 11);
  op2 = util::shift(inst, 5, 7);
  rt = util::shift(inst, 0, 4);

  switch (o0) {
  case 0:
    switch (o1) {
    case 3:
      switch (CRn) {
      case 4:
        switch (op2) {
        case 6:
          LOG_CPU("daifset \n");
          return;
        case 7:
          LOG_CPU("DAIRCLR \n");
          return;
        default:
          unsupported();
          break;
        }
        break;
      default:
        unsupported();
        break;
      }
      break;
    default:
      unsupported();
      break;
    }
    break;
  case 3:
    switch (o1) {
    case 0:
      switch (CRn) {
      case 0:
        switch (CRm) {
        case 0:
          switch (op2) {
          case 5:
            if (!if_get) {
              assert(false);
            }
            xregs[rt] = mpidr_el1;
            LOG_CPU("mrs x%d, MPIDR_EL1(0x%lx)\n", rt, mpidr_el1);
            return;
          default:
            unsupported();
            break;
          }
          break;
        default:
          unsupported();
          break;
        }
        break;
      case 1:
        switch (CRm) {
        case 0:
          switch (op2) {
          case 0:
            if (if_get) {
              xregs[rt] = mmu.sctlr_el1;
              LOG_CPU("mrs x%d, SCTLR_EL1(0x%lx)\n", rt, xregs[rt]);
              return;
            } else {
              mmu.sctlr_el1 = xregs[rt];
              LOG_CPU("msr SCTLR_EL1, x%d(0x%lx)\n", rt, mmu.sctlr_el1);
              return;
            }
            break;
          default:
            unsupported();
            break;
          }
          break;
        default:
          unsupported();
          break;
        }
        break;
      case 2:
        switch (CRm) {
        case 0:
          switch (op2) {
          case 0:
            if (if_get) {
              xregs[rt] = mmu.ttbr0_el1;
              LOG_CPU("mrs x%d, TTBR0_EL1\n", rt);
              return;
            } else {
              mmu.ttbr0_el1 = xregs[rt];
              LOG_CPU("msr TTBR0_EL1, x%d\n", rt);
              return;
            }
            break;
          case 1:
            if (if_get) {
              xregs[rt] = mmu.ttbr1_el1;
              LOG_CPU("mrs x%d, TTBR1_EL1(=0x%lx)\n", rt, xregs[rt]);
              return;
            } else {
              mmu.ttbr1_el1 = xregs[rt];
              LOG_CPU("msr TTBR1_EL1, x%d(=0x%lx)\n", rt, xregs[rt]);
              return;
            }
            break;
          case 2:
            LOG_CPU("TCR_EL1 ");
            if (if_get) {
              xregs[rt] = mmu.tcr_el1.value;
              LOG_CPU("mrs x%d, TCR_EL1(=0x%lx)\n", rt, xregs[rt]);
              return;
            } else {
              mmu.tcr_el1.value = xregs[rt];
              LOG_CPU("msr TCR_EL1, x%d(=0x%lx)\n", rt, xregs[rt]);
              return;
            }
            break;
          default:
            unsupported();
            break;
          }
          break;
        default:
          unsupported();
        }
        break;
      case 4:
        switch (CRm) {
        case 2:
          switch (op2) {
          case 2:
            if (!if_get) {
              assert(false);
            }
            xregs[rt] = CurrentEL;
            LOG_CPU("mrs CurrentEL(=0x%lx)\n", xregs[rt]);
            return;
          default:
            unsupported();
            break;
          }
          break;
        case 6:
          switch (op2){
            case 0:
              xregs[rt] = ICC_PMR_EL1;
              LOG_CPU("msr ICC_PMR_EL1 = 0x%lx\n", xregs[rt]);
              return;
            default:
              unsupported();
              break;
          }
          break;
        default:
          unsupported();
          break;
        }
        break;
      case 6:
        switch (CRm) {
        case 0:
          switch (op2) {
          case 0:
            unsupported();
            break;
          default:
            unsupported();
            break;
          }
          break;
        default:
          unsupported();
          break;
        }
        break;
      case 10:
        switch (CRm) {
        case 2:
          switch (op2) {
          case 0:
            LOG_CPU("MAIR_EL1 \n");
            return;
          default:
            unsupported();
            break;
          }
          break;
        default:
          unsupported();
          break;
        }
        break;
      case 12:
        switch (CRm){
          case 0:
            switch (op2){
              case 0:
                VBAR_EL1 = xregs[rt];
                LOG_CPU("msr VBAR_EL1=0x%lx\n", xregs[rt]);
                return;
              default:
                unsupported();
                break;
            }
            break;
          case 12:
            switch (op2){
              case 5:
                xregs[rt] = ICC_SRE_EL1;
                LOG_CPU("mrs x%d, ICC_SRE_EL1(=0x%lx)\n", rt, ICC_SRE_EL1);
                return;
              case 7:
                ICC_IGRPEN1_EL1 = xregs[rt];
                LOG_CPU("msr ICC_IGRPEN1_EL1 = 0x%lx\n", xregs[rt]);
                return;
              default:
                unsupported();
                break;
            }
            break;
          default:
            unsupported();
            break;
        }
        break;
      default:
        unsupported();
        break;
      }
      break;
    case 3:
      switch (CRn) {
      case 4:
        switch (CRm) {
        case 2:
          switch (op2) {
          case 1:
            if (if_get) {
              LOG_CPU("mrs x%d, daif=0x%lx\n", rt, daif);
              xregs[rt] = daif;
            } else {
              LOG_CPU("msr daif, x%d(=0x%lx)\n", rt, xregs[rt]);
              daif = xregs[rt];
            }
            return;
          default:
            unsupported();
            break;
          }
          break;
        default:
          unsupported();
        }
        break;
      case 14:
        switch (CRm){
          case 0:
            switch (op2){
              case 0:
                xregs[rt] = CNTFRQ_EL0;
                LOG_CPU("mrs x%d, CNTFRQ_EL0(=0x%lx)\n", rt, CNTFRQ_EL0);
                break;
              default:
                unsupported();
                break;
            }
            break;
          case 3:
            switch (op2){
              case 0:
                CNTV_TVAL_EL0 = xregs[rt];
                LOG_CPU("msr CNTV_TVAL_EL0, x%d(=0x%lx)\n", rt, xregs[rt]);
                break;
              case 1:
                xregs[rt]  = CNTV_CTL_EL0;
                LOG_CPU("mrs x%d, CNTV_CTL_EL0(=0x%lx)\n", rt, CNTV_CTL_EL0);
                return;
              default:
                unsupported();
                break;
            }
            break;
          default:
            unsupported();
            break;
        }
        break;
      default:
        unsupported();
        break;
      }
      break;
    default:
      unsupported();
      break;
    }
    break;
  default:
    unsupported();
    break;
  }
  if (if_get) {
    LOG_CPU("mrs\n");
  } else {
    LOG_CPU("msr\n");
  }
}

/*
         System instructions

          31       22  21  2019 18 16  15  12 11  8 7  5 4   0
         +------------+---+----+------+-----+-----+-----+-----+
         | 1101010100 | L | 01 | op1  | CRn | CRm | op2 |  Rt |
         +------------+---+----+------+-----+-----+-----+-----+

         @L:0->SYS, 1->SYSL
*/

void Cpu::decode_system_instructions(uint32_t inst) {
  uint8_t l, op1, crn, crm, op2 /*, rt*/;

  l = util::bit(inst, 21);
  op1 = util::shift(inst, 16, 18);
  crn = util::shift(inst, 12, 15);
  crm = util::shift(inst, 8, 11);
  op2 = util::shift(inst, 5, 7);
  // rt = util::shift(inst, 0, 4);

  /* SYSL */
  if (l) {
    unsupported();
    return;
  }

  /* SYS */
  if ((op1 == 0b000) && (crn == 0b1000) && (crm == 0b0011) && (op2 == 0b000)) {
    LOG_CPU("tlbi vmalle1is\n");
  } else {
    unsupported();
  }
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
  uint8_t opc, op2, op3, Rn, op4, n;
  uint64_t target = pc;

  opc = util::shift(inst, 21, 24);
  op2 = util::shift(inst, 16, 20);
  op3 = util::shift(inst, 10, 15);
  Rn = util::shift(inst, 5, 9);
  op4 = util::shift(inst, 0, 4);

  switch (opc) {
  case 0:
    if (op2 != 0b11111) {
      unallocated();
      return;
    }
    switch (op3) {
    case 0:
      if (op4 != 0) {
        unallocated();
        return;
      }
      LOG_CPU("br x%d(=0x%lx)\n", Rn, xregs[Rn]);
      set_pc(xregs[Rn]);
      return;
    default:
      LOG_CPU("decode_unconditional_branch_reg\n");
      unsupported();
      increment_pc();
      return;
    }
    break;
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
      // LOG_CPU("sp=0x%lx\n", sp);
      if (target == 0) {
        exit(0);
      }
      set_pc(target);
      break;
    default:
      LOG_CPU("decode_unconditional_branch_reg\n");
      unsupported();
      increment_pc();
      return;
    }
    break;
  default:
    LOG_CPU("decode_unconditional_branch_reg\n");
    unsupported();
    increment_pc();
    return;
  }
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
  uint64_t imm19;
  uint8_t op, if_64bit, rt;
  bool if_jump = false;

  if_64bit = util::bit(inst, 31);
  op = util::bit(inst, 24);
  imm19 = util::shift(inst, 5, 23);
  rt = util::shift(inst, 0, 4);

  switch (op) {
  case 0: // CBZ
    LOG_CPU("cbz ");
    if (if_64bit) {
      if_jump = xregs[rt] == 0;
    } else {
      if_jump = (uint32_t)xregs[rt] == 0;
    }
    break;
  case 1: // CBNZ
    LOG_CPU("cbnz ");
    if (if_64bit) {
      if_jump = xregs[rt] != 0;
    } else {
      if_jump = (uint32_t)xregs[rt] != 0;
    }
    break;
  }
  offset = util::SIGN_EXTEND(imm19 << 2, 21);
  LOG_CPU("x%d, 0x%lx\n", rt, pc + offset);
  if (if_jump) {
    set_pc(pc + offset);
  } else {
    increment_pc();
  }
}

/*
         Test and branch (immediate)

           31   30   25  24   23  19  18          5  4      0
         +----+--------+----+-------+---------------+------+
         | b5 | 011011 | op |  b40  |    imm14      |  Rt  |
         +----+--------+----+-------+---------------+------+

         @op: 0->TBZ, 1->TBNZ
         @sh: 0->32bit, 1->64bit
                                 @op: 0->CBZ, 1->CBNZ

*/
void Cpu::decode_test_and_branch_imm(uint32_t inst) {
  uint64_t imm14, offset;
  uint8_t op, rt, b5, b40, bit_pos;
  bool if_jump;

  b5 = util::bit(inst, 31);
  op = util::bit(inst, 24);
  b40 = util::shift(inst, 19, 23);
  imm14 = util::shift(inst, 5, 18);
  rt = util::shift(inst, 0, 4);

  bit_pos = (b5 << 5) | b40;
  if (!b5) {
    assert(bit_pos < 32);
  }

  if_jump = (uint8_t)util::bit64(xregs[rt], bit_pos) == op;
  offset = if_jump ? (imm14 << 2) : 4;

  [[maybe_unused]] const char *name = op ? "tbnz" : "tbz";
  LOG_CPU("%s x%d, #%d, 0x%lx\n", name, rt, bit_pos, pc + offset);

  set_pc(pc + offset);
}
