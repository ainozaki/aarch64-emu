#pragma once

#include <cstdint>

namespace core {

namespace cpu {
struct CPSR;
} // namespace cpu

void set_Nflag(core::cpu::CPSR &cpsr, uint64_t val);
void set_Zflag(core::cpu::CPSR &cpsr, uint64_t val);

uint64_t add_imm(uint64_t x, uint64_t y, uint8_t carry_in);

uint64_t add_imm_s(uint64_t x, uint64_t y, uint8_t carry_in,
                   core::cpu::CPSR &cpsr);

} // namespace core
