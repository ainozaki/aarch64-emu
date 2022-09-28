#include "emulator.h"

#include <iostream>
#include <string>

#include "const.h"

int main(int argc, char **argv, char **envp) {
  if (argc < 2) {
    printf("usage: %s <filename>\n", argv[0]);
    return 0;
  }

  Emulator emu(argc, argv, envp);
  if (emu.init() != SysResult::Success) {
    return -1;
  }
  emu.execute_loop();
}
