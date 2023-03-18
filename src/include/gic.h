#pragma once

#include <cstdint>

/*
 * Redistributor registers, offsets from RD_base
 */
const uint16_t GICR_CTLR = 0x0000;
const uint16_t GICR_TYPER = 0x0008;
class Gic{
public:
    Gic() = default;
    ~Gic() = default;
};