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
  cpu_->execute(0x91004400); /* ADD X0, X0, #0x11 */
  cpu_->execute(0x927c0001); /* AND X1, X0, #0x10 */
  cpu_->execute(0xb27c0002); /* OOR X2, X0, #0x10 */

  return 0;
}
