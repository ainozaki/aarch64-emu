#ifndef OP_H_
#define OP_H_

#include <cstdint>

struct CPSR;

uint64_t add_imm(uint64_t x, uint64_t y, uint8_t carry_in);
uint64_t add_imm_s(uint64_t x, uint64_t y, uint8_t carry_in, struct CPSR &cpsr);

#endif // OP_H_
