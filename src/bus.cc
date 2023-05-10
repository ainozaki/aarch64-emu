#include "bus.h"

#include <string>

#include <stdio.h>
#include <stdlib.h>

#include "loader.h"
#include "log.h"
#include "mem.h"
#include "mmu.h"
#include "utils.h"
#include "virtio.h"

Bus::Bus(uint64_t text_start, uint64_t text_size, uint64_t map_base,
         const std::string &diskname)
    : virtio(Virtio(diskname)) {
  mem.init(text_start, text_size, map_base);
}

uint64_t Bus::load(uint64_t address, MemAccessSize size) {
  if ((address >= gicv3_base) && (address < gicv3_base + gicv3_size)) {
    LOG_CPU("gicv3 address load: 0x%lx\n", address);
    return gic.load(address);
  } else if ((address >= uart_base) && (address <= uart_base + uart_size)) {
    LOG_CPU("uart address load: 0x%lx\n", address);
    return uart.load(address);
  } else if ((address >= virtio_mmio_base) &&
             (address <= virtio_mmio_base + virtio_mmio_size)) {
    LOG_CPU("virtio address load: 0x%lx\n", address);
    return virtio.load(address);
  } else if ((address >= ram_base) && (address <= ram_base + ram_size)) {
    switch (size) {
    case MemAccessSize::Byte:
      return mem.load8(address);
    case MemAccessSize::Hex:
      return mem.load16(address);
    case MemAccessSize::Word:
      return mem.load32(address);
    case MemAccessSize::DWord:
      return mem.load64(address);
    default:
      assert(false);
    }
  } else {
    printf("load unknown address: 0x%lx\n", address);
    exit(0);
  }
  assert(false);
  // dummy
  return 0;
}

void Bus::store(uint64_t address, uint64_t value, MemAccessSize size) {
  if ((address >= gicv3_base) && (address < gicv3_base + gicv3_size)) {
    LOG_CPU("gicv3 address store: 0x%lx\n", address);
    gic.store(address, value);
    return;
  } else if ((address >= uart_base) && (address <= uart_base + uart_size)) {
    LOG_CPU("uart address store: 0x%lx\n", address);
    uart.store(address, value);
    return;
  } else if ((address >= virtio_mmio_base) &&
             (address <= virtio_mmio_base + virtio_mmio_size)) {
    LOG_CPU("virtio mmio address store: 0x%lx\n", address);
    virtio.store(address, value);
    return;
  } else if ((address >= ram_base) && (address <= ram_base + ram_size)) {
    switch (size) {
    case MemAccessSize::Byte:
      mem.store8(address, value);
      break;
    case MemAccessSize::Hex:
      mem.store16(address, value);
      break;
    case MemAccessSize::Word:
      mem.store32(address, value);
      break;
    case MemAccessSize::DWord:
      mem.store64(address, value);
      break;
    default:
      assert(false);
    }
    return;
  } else {
    printf("store unknown address: 0x%lx\n", address);
    exit(0);
  }
}