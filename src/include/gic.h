#pragma once

#include <cstdint>


const uint64_t GIC_DIST = 0x08000000;
const uint64_t GICD_CTLR = GIC_DIST + 0x0;
const uint64_t GICD_TYPER = GIC_DIST + 0x4;
const uint64_t GICD_IGROUPR = GIC_DIST + 0x80;
const uint64_t GICD_ICPENDR = GIC_DIST + 0x280;
const uint64_t GICD_IPRIORITYR = GIC_DIST + 0x400;
const uint64_t GICD_ITARGETSR = GIC_DIST + 0x800;

const uint64_t GIC_REDIST = 0x080a0000;
const uint64_t GICR_CTLR = GIC_REDIST + 0x0;

class Gic{
public:
    Gic() = default;
    ~Gic() = default;

    void store(uint64_t addr, uint64_t value);
    uint64_t load(uint64_t addr);
private:
    uint32_t d_ctlr;
    uint32_t d_typer;
    uint32_t d_igroupr[32];
    uint8_t d_ipriorityr[1024];
    uint32_t r_ctlr;
};