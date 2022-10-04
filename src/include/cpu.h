#pragma once

#include <cstdint>

#include "bus.h"
#include "log.h"

struct CPSR
{
  char buff[27];
  uint8_t Q : 1;
  uint8_t V : 1;
  uint8_t C : 1;
  uint8_t Z : 1;
  uint8_t N : 1;
};

class Cpu
{
public:
  Bus bus;
  uint64_t pc;
  CPSR cpsr; /* Current Program Status Register*/

  /* map SP to xregs[31] */
  uint64_t xregs[32] = {0};
  uint64_t sp_el[4];  /* Stack pointers*/
  uint64_t elr_el[4]; /* Exception Linked Registers */
  const uint64_t xzr = 0;

  Cpu() = default;
  ~Cpu() = default;
  uint32_t fetch();
  void decode_start(uint32_t inst);
  void show_regs();

private:
  void update_lower32(uint8_t reg, uint32_t value);
  void increment_pc() { pc += 4; }
  void set_pc(uint64_t new_pc) { pc = new_pc; }
  bool check_b_flag(uint8_t cond);

  void decode_sme_encodings(uint32_t inst);
  void decode_unallocated(uint32_t inst);
  void decode_sve_encodings(uint32_t inst);
  void decode_loads_and_stores(uint32_t inst);
  void decode_data_processing_imm(uint32_t inst);
  void decode_data_processing_reg(uint32_t inst);
  void decode_data_processing_float(uint32_t inst);
  void decode_branches(uint32_t inst);

  /* loads/stores */
  void decode_ldst_reg_unscaled_immediate(uint32_t inst);
  void decode_ldst_register_pair(uint32_t inst);
  void decode_ldst_register(uint32_t inst);
  void decode_ldst_reg_unsigned_imm(uint32_t inst);
  void decode_ldst_reg_immediate(uint32_t inst);
  void decode_ldst_reg_unpriviledged(uint32_t inst);
  void decode_ldst_atomic_memory_op(uint32_t inst);
  void decode_ldst_reg_pac(uint32_t inst);
  void decode_ldst_reg_reg_offset(uint32_t inst);

  /* Data Processing Register */
  void decode_addsub_shifted_reg(uint32_t inst);
  void decode_addsub_extended_reg(uint32_t inst);
  void decode_logical_shifted_reg(uint32_t inst);
  void decode_conditional_select(uint32_t inst);

  /* Data Processing Immediate */
  void decode_pc_rel(uint32_t inst);
  void decode_add_sub_imm(uint32_t inst);
  void decode_add_sub_imm_with_tags(uint32_t inst);
  void decode_logical_imm(uint32_t inst);
  void decode_move_wide_imm(uint32_t inst);
  void decode_bitfield(uint32_t inst);
  void decode_extract(uint32_t inst);

  /* Branches, Exception Generating and System instructions */
  void decode_conditional_branch_imm(uint32_t inst);
  void decode_exception_generation(uint32_t inst);
  void decode_unconditional_branch_reg(uint32_t inst);
  void decode_unconditional_branch_imm(uint32_t inst);
  void decode_compare_and_branch_imm(uint32_t inst);
};

typedef void (Cpu::*decode_func)(uint32_t inst);