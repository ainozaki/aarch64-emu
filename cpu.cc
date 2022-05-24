#include "cpu.h"

#include <bitset>
#include <iomanip>
#include <iostream>

// Data Processing Immediate
#define OP_ADDSUB_IMM 0x22
#define OP_ADDSUB_IMM_ADD 0
#define OP_ADDSUB_IMM_SUB 1

void Cpu::show_regs() {
  std::cout << "=====================GPregs======================" << std::endl;
  for (int i = 0; i < 31; i++) {
    std::cout << std::setw(2) << i << ":" << std::hex << std::setfill('0')
              << std::right << std::setw(8) << xregs_[i] << "	";
    if (i % 3 == 2) {
      std::cout << std::endl;
    }
  }
  std::cout << std::endl
            << "=================================================" << std::endl;
}

void Cpu::execute(uint32_t inst) {
  uint8_t opcode, datasize;
  uint16_t imm12;
  uint32_t rd, rn;
  uint64_t imm;
  char op, s, sf, sh;

  opcode = (inst >> 23) & ((1 << 7) - 1);
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
      if (op == OP_ADDSUB_IMM_ADD) {
        xregs_[rd] = xregs_[rn] + imm;
      } else {
        xregs_[rd] = xregs_[rn] - imm;
      }
    } else {
      /* ADDS/SUBS */
    }
    break;
  default:
    std::cout << "Unknown opcode: " << std::bitset<6>(opcode) << std::endl;
  }
}
