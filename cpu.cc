#include "cpu.h"

#include <bitset>
#include <iomanip>
#include <iostream>

#include "op.h"

// Data Processing Immediate
#define OP_ADDSUB_IMM 0x22
#define OP_ADDSUB_IMM_ADD 0
#define OP_ADDSUB_IMM_SUB 1

void Cpu::show_regs() {
  std::cout << "=================================================" << std::endl;
  for (int i = 0; i < 31; i++) {
    std::cout << std::setw(2) << i << ":" << std::hex << std::setfill('0')
              << std::right << std::setw(16) << xregs_[i] << "	";
    if (i % 3 == 2) {
      std::cout << std::endl;
    }
  }
  std::cout << std::endl;
  std::cout << "CPSR: [NZCV]=" << cpsr_.N << cpsr_.Z << cpsr_.C << cpsr_.Z
            << std::endl;
  std::cout << "=================================================" << std::endl;
}

uint64_t add_with_s(uint64_t x, uint64_t y, uint8_t carry_in,
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

void Cpu::execute(uint32_t inst) {
  uint8_t opcode = 0, datasize;
  uint16_t imm12;
  uint32_t rd, rn;
  uint64_t imm;
  char op, s, sf, sh;

  opcode = (inst >> 23) & ((1 << 6) - 1);
  std::cout << "opcode: " << std::bitset<6>(opcode) << std::endl;
  switch (opcode) {
  case OP_ADDSUB_IMM:
    rd = inst & ((1 << 5) - 1);
    rn = (inst >> 5) & ((1 << 5) - 1);
    imm12 = (inst >> 10) & ((1 << 12) - 1);
    sh = (inst >> 22) & 1;
    s = (inst >> 29) & 1;
    op = (inst >> 30) & 1;
    sf = (inst >> 31) & 1;

    datasize = (sf == 1) ? 64 : 32;
    imm = imm12;
    if (sh) {
      imm = imm >> 12;
    }

    if (s == 0) {
      /* ADD/SUB */
      if (op == OP_ADDSUB_IMM_ADD) {
        xregs_[rd] = xregs_[rn] + imm;
      } else {
        xregs_[rd] = xregs_[rn] - imm;
      }
    } else {
      /* ADDS/SUBS */
      if (op == OP_ADDSUB_IMM_ADD) {
        xregs_[rd] = add_with_s(xregs_[rn], imm, /*carry-in=*/0, cpsr_);
      } else {
        xregs_[rd] = add_with_s(xregs_[rn], ~imm, /*carry-in=*/1, cpsr_);
      }
    }
    break;
  default:
    std::cout << "Unknown opcode: " << std::bitset<6>(opcode) << std::endl;
  }
  show_regs();
}
