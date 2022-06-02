#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <vector>

#include <arm.h>
#include <mem.h>

#define MEM_SIZE 1024

class Emulator {
public:
  Emulator();
  ~Emulator();

  int Execute();

private:
  Cpu cpu;
  Mem mem;
};
