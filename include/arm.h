#pragma once

#include <cstdint>

struct CPSR {
  char buff[27];
  char Q;
  char V;
  char C;
  char Z;
  char N;
};
class Cpu {
public:
  Cpu() = default;
  ~Cpu() = default;

public:
  void sme_encodings(uint32_t inst);
  void unallocated(uint32_t inst);
  void sve_encodings(uint32_t inst);
  void loads_and_stores(uint32_t inst);
  void data_processing_imm(uint32_t inst);
  void data_processing_reg(uint32_t inst);
  void data_processing_float(uint32_t inst);
  void branches(uint32_t inst);

  void show_regs();
  void execute(uint32_t inst);

  int64_t xregs_[31] = {0};
  uint64_t pc_;
  uint64_t sp_el_[4];  /* Stack pointers*/
  uint64_t elr_el_[4]; /* Exception Linked Registers */

  CPSR cpsr_; /* Current Program Status Register*/
};
