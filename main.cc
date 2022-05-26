#include "emulator.h"

#include <iostream>

int main() {
  const char *filename = "binary.txt";
  Emulator emu(filename);
  emu.Execute();
  std::cout << "Finish emulating" << std::endl;
}
