#include "bus.h"

#include "mem.h"

uint8_t Bus::load8(uint64_t address) { return mem.load8(address); }

uint32_t Bus::load32(uint64_t address) { return mem.load32(address); }

uint64_t Bus::load64(uint64_t address) { return mem.load64(address); }

void Bus::store8(uint64_t address, uint8_t value) {
  mem.store8(address, value);
}

void Bus::store32(uint64_t address, uint32_t value) {
  mem.store32(address, value);
}

void Bus::store64(uint64_t address, uint64_t value) {
  mem.store64(address, value);
}