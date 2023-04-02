#include "gic.h"

#include <stdio.h>

void Gic::store(uint64_t addr, uint64_t value){   
    uint8_t irq;
    if (addr == GICD_CTLR){
        d_ctlr = value;
    }else if (addr ==  GICD_TYPER){
        d_typer = value;
    }else if ( (addr >= GICD_IGROUPR) &&  (addr <= GICD_IGROUPR + 0x7f)){
        irq = addr - GICD_IGROUPR;
        printf("igroupr irq = %d\n", irq);
    }else if ((addr >= GICD_IPRIORITYR) && (addr <= GICD_IPRIORITYR + 0x3ff)){
        irq = addr - GICD_IPRIORITYR;
        for (int i = irq; i < irq + 4; i++, value >>= 8){
            d_ipriorityr[i] = value;
        }
    }else if ((addr >= GICD_ITARGETSR) && (addr <= GICD_ITARGETSR + 0x3ff)){
        // RAZ/WI
    }else {
        printf("gic store 0x%lx\n", addr);
    }
}

uint64_t Gic::load(uint64_t addr){
    uint8_t irq;
    if (addr == GICD_CTLR){
        return d_ctlr;
    }else if (addr ==  GICD_TYPER){
        return d_typer;
    }else if ( (addr >= GICD_IGROUPR) &&  (addr <= GICD_IGROUPR + 0x7f)){
        irq = addr - GICD_IGROUPR;
        printf("igroupr irq = %d\n", irq);
        return 0;
    }else if ((addr >= GICD_IPRIORITYR) && (addr <= GICD_IPRIORITYR + 0x3ff)){
        irq = addr - GICD_IPRIORITYR;
        uint32_t value = 0;
        for (int i = irq + 3; i >= irq; i--){
            value <<= 8;
            value |= d_ipriorityr[i];
        }
        return value;
    }else if ((addr >= GICD_ITARGETSR) && (addr <= GICD_ITARGETSR + 0x3ff)){
        // RAZ/WI
        return 0;
    }else {
        printf("gic load 0x%lx\n", addr);
        return 0;
    }
}