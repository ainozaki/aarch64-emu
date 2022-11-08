#include "emulator.h"

#include <iostream>
#include <string>

#include "const.h"

int main(int argc, char **argv, char **envp) {
  int err;

  if (argc < 2) {
    printf("usage: %s <filename>\n", argv[0]);
    return 0;
  }

  Emulator emu(argc, argv, envp);
  if ((err = emu.init()) != ESUCCESS) {
    printf("Emulator initialization failed.\n");
    return err;
  }

  emu.execute_loop();
  return 0;
}
