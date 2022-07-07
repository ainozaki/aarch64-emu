#pragma once

#include <cstdint>

namespace core {

class System;

namespace cpu {

struct CPSR {
  char buff[27];
  uint8_t Q : 1;
  uint8_t V : 1;
  uint8_t C : 1;
  uint8_t Z : 1;
  uint8_t N : 1;
};

class Cpu {
public:
  Cpu(System *system);
  ~Cpu() = default;

  void update_lower32(uint8_t reg, uint32_t value);
	void increment_pc(){pc += 4;}
	void set_pc(uint64_t new_pc){pc = new_pc;}

  uint64_t pc;

public:
  void show_regs();

  /* map SP to xregs[31] */
  uint64_t xregs[32] = {0};
  uint64_t sp_el[4];  /* Stack pointers*/
  uint64_t elr_el[4]; /* Exception Linked Registers */

  CPSR cpsr; /* Current Program Status Register*/

  System *system_;
};

} // namespace cpu
} // namespace core
