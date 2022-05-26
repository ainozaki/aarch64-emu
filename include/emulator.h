#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <vector>

#define MEM_SIZE 1024

class Cpu;

class Emulator {
public:
  Emulator(const char *filename);
  ~Emulator();

  int Execute();

private:
  std::ifstream f_;
  std::unique_ptr<Cpu> cpu_;
  uint8_t memory_[MEM_SIZE];
};
