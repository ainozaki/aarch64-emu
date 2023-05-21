#pragma once

#include <cstdint>

const uint8_t UART_RX_BUFF_LEN = 64;

class Uart {
public:
  void store(uint64_t addr, uint64_t value);
  uint64_t load(uint64_t addr);

  bool is_interrupting();
  uint16_t uart_dr;
  uint8_t uart_rx_idx = 0;
  uint8_t uart_rx_last_idx = 0;
  uint8_t uart_rx_buff[UART_RX_BUFF_LEN];

private:
  uint16_t uart_fr = 0x90;
  uint16_t uart_lcr_h;
  uint16_t uart_cr;
  uint16_t uart_imsc;
  uint16_t uart_icr = 0x10;

  uint64_t counter = 0;
};
