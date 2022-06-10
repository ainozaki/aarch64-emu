#include "arm.h"

#include <bitset>
#include <cassert>
#include <iomanip>
#include <iostream>

#include "arm_op.h"
#include "system.h"
#include "utils.h"

namespace core {

namespace cpu {

Cpu::Cpu(System *system) : system_(system) {}

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

void Cpu::update_lower32(uint8_t reg, uint32_t value) {
  xregs[reg] = (xregs[reg] & (uint64_t)0xffffffff << 32) | value;
}

} // namespace cpu

} // namespace core
