#pragma once

#include <cstdint>
#include <memory>

#include "bus.h"
#include "log.h"
#include "mmu.h"

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
  Bus bus;
  MMU mmu;
  uint64_t pc;
  CPSR cpsr; /* Current Program Status Register*/

  uint64_t xregs[32] = {0};
  uint64_t sp;
  uint64_t elr_el[4]; /* Exception Linked Registers */
  const uint64_t xzr = 0;
  uint64_t CurrentEL;
  const uint64_t mpidr_el1 = 0x80000000;
  uint64_t VBAR_EL1;

  /* PSTATE */
  /*
  DAIF, Interrupt Mask Bits
  Allows access to the interrupt mask bits.
  D[9]: Debug
  A[8]: SError interrupt mask
  I[7]: IRQ mask
  F[6]: FIQ mask
  0: not masked, 1: masked
  */
  uint64_t daif = 0x3c0;

  // Interrupt
  uint64_t
      ICC_IGRPEN1_EL1; // Interrupt Controller Interrupt Group 1 Enable register
  uint64_t ICC_PMR_EL1; // Interrupt Controller Interrupt Priority Mask Register
  uint64_t ICC_SRE_EL1 =
      0x7; // Interrupt Controller System Register Enable register (EL1)

  // Timer
  uint64_t CNTV_CTL_EL0 = 0; // Counter-timer Virtual Timer Control register
  uint64_t CNTFRQ_EL0 = 0x3b9aca0; // Counter-timer Frequency register
  uint64_t CNTV_TVAL_EL0 = 0; // Counter-timer Virtual Timer TimerValue register

  Cpu() = default;
  ~Cpu() = default;
  void init(uint64_t pc, uint64_t sp, uint64_t text_start, uint64_t text_size,
            uint64_t map_base);
  void check_interrupt();
  uint32_t fetch();
  void decode_start(uint32_t inst);
  void show_regs();
  void show_stack();

private:
  void increment_pc() { pc += 4; }
  void set_pc(uint64_t new_pc) { pc = new_pc; }
  bool check_b_flag(uint8_t cond);

  uint64_t load(uint64_t address, MemAccessSize size);
  void store(uint64_t address, uint64_t value, MemAccessSize size);

  void unsupported();
  void unallocated();

  void decode_sme_encodings(uint32_t inst);
  void decode_unallocated(uint32_t inst);
  void decode_sve_encodings(uint32_t inst);
  void decode_loads_and_stores(uint32_t inst);
  void decode_data_processing_imm(uint32_t inst);
  void decode_data_processing_reg(uint32_t inst);
  void decode_data_processing_float(uint32_t inst);
  void decode_data_processing_3source(uint32_t inst);
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
  void decode_ldst_load_register_literal(uint32_t inst);
  void decode_ldst_exclusive(uint32_t inst);
  void decode_ldst_ordered(uint32_t inst);

  /* Data Processing Register */
  void decode_addsub_shifted_reg(uint32_t inst);
  void decode_addsub_extended_reg(uint32_t inst);
  void decode_logical_shifted_reg(uint32_t inst);
  void decode_conditional_select(uint32_t inst);
  void decode_data_processing_1source(uint32_t inst);
  void decode_data_processing_2source(uint32_t inst);

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
  void decode_system_register_move(uint32_t inst);
  void decode_system_instructions(uint32_t inst);
  void decode_pstate(uint32_t inst);
  void decode_barriers(uint32_t inst);
  void decode_unconditional_branch_reg(uint32_t inst);
  void decode_unconditional_branch_imm(uint32_t inst);
  void decode_compare_and_branch_imm(uint32_t inst);
  void decode_test_and_branch_imm(uint32_t inst);
  void impl_sysop(uint8_t op);
};

typedef void (Cpu::*decode_func)(uint32_t inst);