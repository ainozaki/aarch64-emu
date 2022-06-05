#pragma once

#include <cstdint>

class Cpu;

typedef void (*decode_func)(uint32_t inst, Cpu *cpu);

void decode_sme_encodings(uint32_t inst, Cpu *cpu);
void decode_unallocated(uint32_t inst, Cpu *cpu);
void decode_sve_encodings(uint32_t inst, Cpu *cpu);

/*
         loads/stores
*/
void decode_loads_and_stores(uint32_t inst, Cpu *cpu);
void decode_ldst_register(uint32_t inst, Cpu *cpu);
void decode_ldst_reg_unscaled_imm(uint32_t inst, Cpu *cpu);
void decode_ldst_reg_imm_post_indexed(uint32_t inst, Cpu *cpu);
void decode_ldst_reg_unpriviledged(uint32_t inst, Cpu *cpu);
void decode_ldst_reg_imm_pre_indexed(uint32_t inst, Cpu *cpu);
void decode_ldst_atomic_memory_op(uint32_t inst, Cpu *cpu);
void decode_ldst_reg_pac(uint32_t inst, Cpu *cpu);
void decode_ldst_reg_reg_offset(uint32_t inst, Cpu *cpu);

const decode_func decode_ldst_reg_tbl[] = {
    decode_ldst_reg_unscaled_imm,  decode_ldst_reg_imm_post_indexed,
    decode_ldst_reg_unpriviledged, decode_ldst_reg_imm_pre_indexed,
    decode_ldst_atomic_memory_op,  decode_ldst_reg_pac,
    decode_ldst_reg_reg_offset,    decode_ldst_reg_pac,
};

void decode_data_processing_imm(uint32_t inst, Cpu *cpu);
void decode_data_processing_reg(uint32_t inst, Cpu *cpu);
void decode_data_processing_float(uint32_t inst, Cpu *cpu);
void decode_branches(uint32_t inst, Cpu *cpu);

/*
         Data Processing Immediate
*/
void decode_pc_rel(uint32_t inst, Cpu *cpu);
void decode_add_sub_imm(uint32_t inst, Cpu *cpu);
void decode_add_sub_imm_with_tags(uint32_t inst, Cpu *cpu);
void decode_logical_imm(uint32_t inst, Cpu *cpu);
void decode_move_wide_imm(uint32_t inst, Cpu *cpu);
void decode_bitfield(uint32_t inst, Cpu *cpu);
void decode_extract(uint32_t inst, Cpu *cpu);
