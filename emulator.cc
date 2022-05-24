#include "emulator.h"

#include <iostream>
#include <memory>

#include "cpu.h"

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
  cpu_->execute(0x91048c20);
  cpu_->show_regs();
  return 0;
}

int main() {
  const char *filename = "binary.txt";
  Emulator emu(filename);
  emu.Execute();
  std::cout << "Finish emulating" << std::endl;
}
