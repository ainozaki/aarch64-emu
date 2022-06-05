#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <vector>

#include <arm.h>
#include <mem.h>

#define MEM_SIZE 1024

namespace core {

class System {
public:
  System();
  ~System();

  void Init();

  int Execute();

  cpu::Cpu &cpu() { return cpu_; }
  mem::Mem &mem() { return mem_; }

private:
  cpu::Cpu cpu_;
  mem::Mem mem_;
};

} // namespace core
