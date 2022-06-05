#include "arm.h"

#include <bitset>
#include <cassert>
#include <iomanip>
#include <iostream>

#include "arm_decoder.h"
#include "arm_op.h"
#include "system.h"
#include "utils.h"

namespace core {

namespace cpu {

Cpu::Cpu(System *system) : decoder_(decode::Decoder(system)), system_(system) {}

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

void Cpu::execute(uint32_t inst) { decoder_.start(inst); }

} // namespace cpu

} // namespace core
