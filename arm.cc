#include "arm.h"

#include <bitset>
#include <cassert>
#include <iomanip>
#include <iostream>

#include "arm-op.h"
#include "decode.h"
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

const decode_func decode_inst_tbl[] = {
    decode_sme_encodings,
    decode_unallocated,
    decode_sve_encodings,
    decode_unallocated,
    decode_loads_and_stores,
    decode_data_processing_reg,
    decode_loads_and_stores,
    decode_data_processing_float,
    decode_data_processing_imm,
    decode_data_processing_imm,
    decode_branches,
    decode_branches,
    decode_loads_and_stores,
    decode_data_processing_reg,
    decode_loads_and_stores,
    decode_data_processing_float,
};

void Cpu::execute(uint32_t inst) {
  uint8_t op1;

  op1 = bitutil::shift(inst, 25, 28);
  decode_inst_tbl[op1](inst, this);
}
