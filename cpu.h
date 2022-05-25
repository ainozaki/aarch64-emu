#ifndef CPU_H_
#define CPU_H_

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

  void show_regs();
  void execute(uint32_t inst);

private:
  uint64_t xregs_[31];
  uint64_t pc_;
  uint64_t sp_el_[4];  /* Stack pointers*/
  uint64_t elr_el_[4]; /* Exception Linked Registers */

  struct CPSR cpsr_; /* Current Program Status Register*/
};

#endif // CPU_H_
