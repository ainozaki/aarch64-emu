#pragma once

#include <cstdint>

struct CPSR;

int64_t add_imm(int64_t x, int64_t y, int8_t carry_in);
int64_t add_imm_s(int64_t x, int64_t y, int8_t carry_in, CPSR &cpsr);
int64_t and_imm_s(int64_t x, int64_t y, CPSR &cpsr);
