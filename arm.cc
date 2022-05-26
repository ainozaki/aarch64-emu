#include "arm.h"

#include <bitset>
#include <iomanip>
#include <iostream>

#include "arm-op.h"
#include "utils.h"

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

void Cpu::data_processing_imm(uint32_t inst) {
  uint8_t op0, datasize;
  int16_t imm12;
  uint32_t rd, rn;
  int64_t imm;
  char op, s, sf, sh;

  op0 = bitutil::shift(inst, 23, 25);
  // std::cout << "op0(inst[23:25]): " << std::bitset<6>(op0) << std::endl;
  switch (op0) {
  case 0b010:
    rd = bitutil::shift(inst, 0, 4);
    rn = bitutil::shift(inst, 5, 9);
    imm12 = bitutil::shift(inst, 10, 21);
    sh = bitutil::bit(inst, 22);
    s = bitutil::bit(inst, 29);
    op = bitutil::bit(inst, 30);
    sf = bitutil::bit(inst, 31);

    datasize = (sf == 1) ? 64 : 32;
    imm = bitutil::zero_extend(imm12, 12);
    if (sh) {
      imm = imm >> 12;
    }

    if (s == 0) {
      if (op == 0) {
        xregs_[rd] = add_imm(xregs_[rn], imm, /*carry-in=*/0); /* ADD */
      } else {
        xregs_[rd] = add_imm(xregs_[rn], ~imm, /*carry-in=*/1); /* SUB */
      }
    } else {
      if (op == 0) {
        xregs_[rd] =
            add_imm_s(xregs_[rn], imm, /*carry-in=*/0, cpsr_); /* ADDS */
      } else {
        xregs_[rd] =
            add_imm_s(xregs_[rn], ~imm, /*carry-in=*/1, cpsr_); /* SUBS */
      }
    }
    break;
  default:
    std::cout << "Unknown op0: " << std::bitset<6>(op0) << std::endl;
  }
}

void Cpu::sme_encodings(uint32_t inst){};
void Cpu::unallocated(uint32_t inst){};
void Cpu::sve_encodings(uint32_t inst){};
void Cpu::loads_and_stores(uint32_t inst){};
void Cpu::data_processing_reg(uint32_t inst){};
void Cpu::data_processing_float(uint32_t inst){};
void Cpu::branches(uint32_t inst){};

void Cpu::execute(uint32_t inst) {
  uint8_t op1;

  op1 = bitutil::shift(inst, 25, 28);
  switch (op1) {
  case 0b0000:
    sme_encodings(inst);
    break;
  case 0b0001:
  case 0b0011:
    unallocated(inst);
    break;
  case 0b0010:
    sve_encodings(inst);
    break;
  case 0b0100:
  case 0b0110:
  case 0b1100:
  case 0b1110:
    loads_and_stores(inst);
    break;
  case 0b0101:
  case 0b1101:
    data_processing_reg(inst);
    break;
  case 0b0111:
  case 0b1111:
    data_processing_float(inst);
    break;
  case 0b1000:
  case 0b1001:
    data_processing_imm(inst);
    break;
  case 0b1010:
  case 0b1011:
    branches(inst);
    break;
  }
  // show_regs();
}
