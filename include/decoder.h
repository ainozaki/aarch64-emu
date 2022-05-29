#pragma once

#include <cstdint>

class Cpu;

typedef void (*decode_func)(uint32_t inst, Cpu *cpu);

void decode_sme_encodings(uint32_t inst, Cpu *cpu);
void decode_unallocated(uint32_t inst, Cpu *cpu);
void decode_sve_encodings(uint32_t inst, Cpu *cpu);
void decode_loads_and_stores(uint32_t inst, Cpu *cpu);
void decode_data_processing_imm(uint32_t inst, Cpu *cpu);
void decode_data_processing_reg(uint32_t inst, Cpu *cpu);
void decode_data_processing_float(uint32_t inst, Cpu *cpu);
void decode_branches(uint32_t inst, Cpu *cpu);

/* Data Processing Immediate */
void decode_pc_rel(uint32_t inst, Cpu *cpu);
void decode_add_sub_imm(uint32_t inst, Cpu *cpu);
void decode_add_sub_imm_with_tags(uint32_t inst, Cpu *cpu);
void decode_logical_imm(uint32_t inst, Cpu *cpu);
void decode_move_wide_imm(uint32_t inst, Cpu *cpu);
void decode_bitfield(uint32_t inst, Cpu *cpu);
void decode_extract(uint32_t inst, Cpu *cpu);
