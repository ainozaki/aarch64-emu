#include "emulator.h"

#include <iostream>
#include <memory>

#include "arm.h"

Emulator::Emulator(const char *filename) {
  cpu_ = std::make_unique<Cpu>(Cpu());

  f_.open(filename, std::ios_base::in | std::ios_base::binary);
  if (!f_) {
    std::cerr << "Cannot open " << filename << std::endl;
    return;
  }

  std::cout << "Initialize Emulator" << std::endl;
}

Emulator::~Emulator() { f_.close(); }

int Emulator::Execute() {
  char c;
  cpu_->execute(0x91004001); /* ADD X1, X0, #0x10*/
  cpu_->execute(0xd1000421); /* SUB X1, X1, #0x1 */
  return 0;
}
