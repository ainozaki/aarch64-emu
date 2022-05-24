#ifndef CPU_H_
#define CPU_H_

#include <cstdint>

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
};

#endif // CPU_H_
