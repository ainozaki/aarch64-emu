#ifndef OP_H_
#define OP_H_

struct CPSR;

uint64_t add_with_s(uint64_t x, uint64_t y, uint8_t carry_in, struct CPSR &cpsr);

#endif // OP_H_
