#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <vector>

#include <arm.h>
#include <const.h>
#include <mem.h>

#define MEM_SIZE 1024

namespace core {

class System {
public:
  System(const char *filename);
  ~System();

  SystemResult Init();

  int Execute();
	void execute_loop();
	void decode_start(uint32_t inst);

  cpu::Cpu &cpu() { return cpu_; }
  mem::Mem &mem() { return mem_; }

private:
	uint32_t fetch();
  
	cpu::Cpu cpu_;
  mem::Mem mem_;
	const char *filename_;

  void decode_sme_encodings(uint32_t inst);
  void decode_unallocated(uint32_t inst);
  void decode_sve_encodings(uint32_t inst);
	void decode_loads_and_stores(uint32_t inst);
  void decode_data_processing_imm(uint32_t inst);
  void decode_data_processing_reg(uint32_t inst);
  void decode_data_processing_float(uint32_t inst);
  void decode_branches(uint32_t inst);

  /* loads/stores */
  void decode_ldst_register(uint32_t inst);
  void decode_ldst_reg_unsigned_imm(uint32_t inst);
  void decode_ldst_reg_immediate(uint32_t inst);
  void decode_ldst_reg_unpriviledged(uint32_t inst);
  void decode_ldst_atomic_memory_op(uint32_t inst);
  void decode_ldst_reg_pac(uint32_t inst);
  void decode_ldst_reg_reg_offset(uint32_t inst);

  /* Data Processing Immediate */
  void decode_pc_rel(uint32_t inst);
  void decode_add_sub_imm(uint32_t inst);
  void decode_add_sub_imm_with_tags(uint32_t inst);
  void decode_logical_imm(uint32_t inst);
  void decode_move_wide_imm(uint32_t inst);
  void decode_bitfield(uint32_t inst);
  void decode_extract(uint32_t inst);

	/* Branches, Exception Generating and System instructions */
	void decode_unconditional_branch_imm(uint32_t inst);
};

typedef void (System::*decode_func)(uint32_t inst);

} // namespace core
