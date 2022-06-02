#include "emulator.h"

#include <iostream>
#include <memory>

#include "arm.h"
#include "mem.h"

Emulator::Emulator() {
  const char *file = "text";
  uint64_t load_offset = 0x0;

  printf("emu: start emulating\n");
  mem.initialize_mainmem(file, load_offset);
}

Emulator::~Emulator() { printf("emu: finish emulating\n"); }

int Emulator::Execute() {
  cpu.execute(0x91004400); /* ADD X0, X0, #0x11 */
  cpu.execute(0x927c0001); /* AND X1, X0, #0x10 */
  cpu.execute(0xb27c0002); /* OOR X2, X0, #0x10 */

  return 0;
}
