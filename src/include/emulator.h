#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "const.h"
#include "cpu.h"
#include "loader.h"

class Emulator {
public:
  std::unique_ptr<Cpu> cpu;
  Loader loader;

  Emulator(int argc, char **argv, char **envp, const std::string &disk);
  void execute_loop();
  void log_pc(uint64_t addr, const char *msg, uint64_t idx);
  bool init_done_;

private:
  char *filename_;
};
