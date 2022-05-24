#include "cpu.h"

#include <bitset>
#include <iomanip>
#include <iostream>

void Cpu::show_regs() {
  std::cout
      << "===============================GPregs================================"
      << std::endl;
  for (int i = 0; i < 31; i++) {
    std::cout << std::setw(2) << i << ": " << std::bitset<64>(xregs_[i])
              << std::endl;
  }
  std::cout
      << "====================================================================="
      << std::endl;
}
