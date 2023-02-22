#pragma once

#include <cstdint>

#include "const.h"

class Uart {
public:
  Uart() = default;
  ~Uart() = default;

  void store(uint64_t addr, uint64_t value);
  uint64_t load(uint64_t addr);

private:
  uint16_t uart_dr;
  uint16_t uart_fr;
  uint16_t uart_lcr_h;
  uint16_t uart_cr;
  uint16_t uart_imsc;
};
