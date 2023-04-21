#include "gic.h"

#include <cassert>
#include <stdio.h>
#include <stdlib.h>

void Gic::store(uint64_t addr, uint64_t value) {
  uint8_t irq;
  if (addr == GICD_CTLR) {
    d_ctlr = value;
  } else if (addr == GICD_TYPER) {
    d_typer = value;
  } else if ((addr >= GICD_IGROUPR) && (addr <= GICD_IGROUPR + 0x7f)) {
    irq = (addr - GICD_IGROUPR) / 32;
    assert(irq < 32);
    d_igroupr[irq] = value;
  } else if ((addr >= GICD_ISENABLER) && (addr <= GICD_ISENABLER + 0x7f)) {
    irq = (addr - GICD_ISENABLER) / 32;
    assert(irq < 32);
    d_isenabler[irq] = value;
  } else if ((addr >= GICD_ICPENDR) && (addr <= GICD_ICPENDR + 0x7f)) {
    irq = (addr - GICD_ICPENDR) / 32;
    assert(irq < 32);
    d_icpendr[irq] = value;
  } else if ((addr >= GICD_IPRIORITYR) && (addr <= GICD_IPRIORITYR + 0x3ff)) {
    irq = addr - GICD_IPRIORITYR;
    for (int i = irq; i < irq + 4; i++, value >>= 8) {
      d_ipriorityr[i] = value;
    }
  } else if ((addr >= GICD_ITARGETSR) && (addr <= GICD_ITARGETSR + 0x3ff)) {
    // RAZ/WI
  } else if (addr == GICR_CTLR) {
    r_ctlr = value;
  } else if (addr == GICR_WAKER) {
    r_waker = value;
  } else if (addr == GICR_IGROUPR0) {
    r_igroupr0 = value;
  } else if (addr == GICR_ISENABLER0) {
    r_isenabler0 = value;
  } else if (addr == GICR_ICPENDR0) {
    r_icpendr0 = value;
  } else if ((addr >= GICD_IPRIORITYR) && (addr <= GICD_IPRIORITYR + 0x1f)) {
    int idx = (addr - GICD_IPRIORITYR) / 4;
    assert(idx < 8);
    r_ipriorityr[idx] = value;
  } else if (addr == GICR_IGRPMODR0) {
    r_igrpmodr0 = value;
  } else {
    printf("gic unknown store 0x%lx\n", addr);
  }
}

uint64_t Gic::load(uint64_t addr) {
  uint8_t irq;
  if (addr == GICD_CTLR) {
    return d_ctlr;
  } else if (addr == GICD_TYPER) {
    return d_typer;
  } else if ((addr >= GICD_IGROUPR) && (addr <= GICD_IGROUPR + 0x7f)) {
    irq = (addr - GICD_IGROUPR) / 32;
    assert(irq < 32);
    return d_igroupr[irq];
  } else if ((addr >= GICD_ISENABLER) && (addr <= GICD_ISENABLER + 0x7f)) {
    irq = (addr - GICD_ISENABLER) / 32;
    assert(irq < 32);
    return d_isenabler[irq];
  } else if ((addr >= GICD_ICPENDR) && (addr <= GICD_ICPENDR + 0x7f)) {
    irq = (addr - GICD_ICPENDR) / 32;
    assert(irq < 32);
    return d_icpendr[irq];
  } else if ((addr >= GICD_IPRIORITYR) && (addr <= GICD_IPRIORITYR + 0x3ff)) {
    irq = addr - GICD_IPRIORITYR;
    uint32_t value = 0;
    for (int i = irq + 3; i >= irq; i--) {
      value <<= 8;
      value |= d_ipriorityr[i];
    }
    return value;
  } else if ((addr >= GICD_ITARGETSR) && (addr <= GICD_ITARGETSR + 0x3ff)) {
    // RAZ/WI
    return 0;
  } else if (addr == GICR_CTLR) {
    return r_ctlr;
  } else if (addr == GICR_WAKER) {
    return r_waker;
  } else if (addr == GICR_IGROUPR0) {
    return r_igroupr0;
  } else if (addr == GICR_ISENABLER0) {
    return r_isenabler0;
  } else if (addr == GICR_ICPENDR0) {
    return r_icpendr0;
  } else if ((addr >= GICD_IPRIORITYR) && (addr <= GICD_IPRIORITYR + 0x1f)) {
    int idx = (addr - GICD_IPRIORITYR) / 4;
    assert(idx < 8);
    return r_ipriorityr[idx];
  } else if (addr == GICR_IGRPMODR0) {
    return r_igrpmodr0;
  } else {
    printf("gic unknown load 0x%lx\n", addr);
    return 0;
  }
}