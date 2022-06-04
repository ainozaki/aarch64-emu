#pragma once

#include <cstdint>

struct CPSR;

void set_Nflag(CPSR &cpsr, uint64_t val);
void set_Zflag(CPSR &cpsr, uint64_t val);

uint64_t add_imm(uint64_t x, uint64_t y, uint8_t carry_in);

uint64_t add_imm_s(uint64_t x, uint64_t y, uint8_t carry_in, CPSR &cpsr);
