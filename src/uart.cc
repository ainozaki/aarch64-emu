#include "uart.h"

#include <stdio.h>
#include <stdlib.h>

#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "log.h"
#include "utils.h"

bool Uart::is_interrupting() {
  counter++;
  uint8_t rx_intr_mask = 0b10000;
  if (counter > 1000) {
    counter = 0;
    if (!(uart_imsc & rx_intr_mask)) {
      return false;
    }
    if (!(uart_icr & rx_intr_mask)) {
      return false;
    }
    if (uart_rx_idx != uart_rx_last_idx) {
      uart_dr = uart_rx_buff[uart_rx_last_idx++];
      // uart_imsc &= ~rx_intr_mask;
      uart_icr &= ~rx_intr_mask;
      uart_fr &= 0xef;
      return true;
    }
  }
  return false;
}

void Uart::store(uint64_t addr, uint64_t value) {
  uint64_t offset = addr & 0xfff;
  switch (offset) {
  case 0x000:
    uart_dr = value;
    putc((uint8_t)value, stdout);
    fflush(stdout);
    break;
  case 0x018:
    uart_fr = value;
    LOG_CPU("uart_fr = 0x%lx\n", value);
    break;
  case 0x02c:
    uart_lcr_h = value;
    LOG_CPU("uart lcr_h = 0x%lx\n", value);
    break;
  case 0x030:
    uart_cr = value;
    LOG_CPU("uart_cr = 0x%lx\n", value);
    break;
  case 0x038:
    uart_imsc = value;
    LOG_CPU("uart_imsc = 0x%lx\n", value);
    break;
  case 0x44:
    uart_icr = value;
    LOG_CPU("uart_icr = 0x%lx\n", value);
    break;
  default:
    LOG_SYSTEM("uart unsupported store 0x%lx\n", offset);
  }
}

uint64_t Uart::load(uint64_t addr) {
  uint64_t offset = addr & 0xfff;
  uint64_t value;
  switch (offset) {
  case 0x000:
    LOG_CPU("uart_dr load 0x%x\n", uart_dr);
    value = uart_dr;
    uart_dr = 0;
    uart_fr |= (1 << 4);
    return value;
  case 0x018:
    LOG_CPU("uart_fr load 0x%x\n", uart_fr);
    return uart_fr;
  case 0x02c:
    LOG_CPU("uart lcr_h load 0x%x\n", uart_lcr_h);
    return uart_lcr_h;
  case 0x030:
    LOG_CPU("uart_cr load 0x%x\n", uart_cr);
    return uart_cr;
  case 0x038:
    LOG_CPU("uart_imsc load 0x%x\n", uart_imsc);
    return uart_imsc;
  case 0x044:
    LOG_CPU("uart_icr load 0x%x\n", uart_icr);
    return uart_icr;
  default:
    LOG_SYSTEM("uart unsupported load 0x%lx\n", offset);
    return 1;
  }
}