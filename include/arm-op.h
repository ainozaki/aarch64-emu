#pragma once

#include <cstdint>

class Cpu;

/* Data Processing Immediate */
void disassemble_pc_rel(uint32_t inst);
void disassemble_add_sub_imm(uint32_t inst, Cpu *cpu);
void disassemble_add_sub_imm_with_tags(uint32_t inst);
void disassemble_logical_imm(uint32_t inst, Cpu *cpu);
void disassemble_move_wide_imm(uint32_t inst);
void disassemble_bitfield(uint32_t inst);
void disassemble_extract(uint32_t inst);
