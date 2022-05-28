#include "arm.h"

#include <bitset>
#include <cassert>
#include <iomanip>
#include <iostream>

#include "arm-op.h"
#include "utils.h"

void Cpu::show_regs() {
  std::cout << "=================================================" << std::endl;
  for (int i = 0; i < 31; i++) {
    std::cout << std::setw(2) << i << ":" << std::hex << std::setfill('0')
              << std::right << std::setw(16) << xregs[i] << "	";
    if (i % 3 == 2) {
      std::cout << std::endl;
    }
  }
  std::cout << std::endl;
  std::cout << "CPSR: [NZCV]=" << cpsr.N << cpsr.Z << cpsr.C << cpsr.Z
            << std::endl;
  std::cout << "=================================================" << std::endl;
}

void Cpu::disassemble_data_processing_imm(uint32_t inst) {
  uint8_t op0;
  op0 = bitutil::shift(inst, 23, 25);

  switch (op0) {
  case 0b000:
  case 0b001:
    disassemble_pc_rel(inst);
    break;
  case 0b010:
    disassemble_add_sub_imm(inst, this);
    break;
  case 0b011:
    disassemble_add_sub_imm_with_tags(inst);
    break;
  case 0b100:
    disassemble_logical_imm(inst, this);
    break;
  case 0b101:
    disassemble_move_wide_imm(inst);
    break;
  case 0b110:
    disassemble_bitfield(inst);
    break;
  case 0b111:
    disassemble_extract(inst);
    break;
  default:
    fprintf(stderr, "Unknown opcode\n");
    assert(false);
  }
}

void Cpu::disassemble_sme_encodings(uint32_t inst){};
void Cpu::disassemble_unallocated(uint32_t inst){};
void Cpu::disassemble_sve_encodings(uint32_t inst){};
void Cpu::disassemble_loads_and_stores(uint32_t inst){};
void Cpu::disassemble_data_processing_reg(uint32_t inst){};
void Cpu::disassemble_data_processing_float(uint32_t inst){};
void Cpu::disassemble_branches(uint32_t inst){};

void Cpu::execute(uint32_t inst) {
  uint8_t op1;

  op1 = bitutil::shift(inst, 25, 28);
  switch (op1) {
  case 0b0000:
    disassemble_sme_encodings(inst);
    break;
  case 0b0001:
  case 0b0011:
    disassemble_unallocated(inst);
    break;
  case 0b0010:
    disassemble_sve_encodings(inst);
    break;
  case 0b0100:
  case 0b0110:
  case 0b1100:
  case 0b1110:
    disassemble_loads_and_stores(inst);
    break;
  case 0b0101:
  case 0b1101:
    disassemble_data_processing_reg(inst);
    break;
  case 0b0111:
  case 0b1111:
    disassemble_data_processing_float(inst);
    break;
  case 0b1000:
  case 0b1001:
    disassemble_data_processing_imm(inst);
    break;
  case 0b1010:
  case 0b1011:
    disassemble_branches(inst);
    break;
  }
  // show_regs();
}
