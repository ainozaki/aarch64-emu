#include "system.h"

#include <iostream>
#include <memory>

#include <stdio.h>

#include "arm.h"
#include "mem.h"

namespace core {

System::System() : cpu_(cpu::Cpu(this)), mem_(mem::Mem(this)) {
  // Initialize
  Init();

  // execute
  Execute();

  // free
  mem_.clean_mem();
}

System::~System() { printf("emu: finish emulating\n"); }

void System::Init() {
  int err;
  const char *file = "raw-binary";

  printf("emu: start emulating\n");

  /// mem
  err = mem_.init_mem(file);
  if (err != 0) {
    fprintf(stderr, "emu: failed to initialize\n");
    return;
  }
}

int System::Execute() {
  // cpu_.execute(0x91004400); /* ADD X0, X0, #0x11 */
  // cpu_.execute(0x927c0001); /* AND X1, X0, #0x10 */
  // cpu_.execute(0xb27c0002); /* OOR X2, X0, #0x10 */

  cpu_.execute(0xf84043e0); /* LDR X0, [SP, #4] */
  cpu_.execute(0xf8616be0); /* LDR X0, [SP, X1] */

  /*
uint8_t data;
uint64_t addr;
addr = 100;
mem_.write_8(addr, 0x11);
data = mem_.read_8(addr);
printf("emu: mem[%lu] = %d\n", addr, data);
  */
  return 0;
}

} // namespace core
