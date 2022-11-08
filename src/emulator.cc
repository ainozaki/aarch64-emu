#include "emulator.h"

#include <cassert>
#include <iostream>
#include <memory>

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "cpu.h"
#include "loader.h"
#include "log.h"
#include "mem.h"
#include "utils.h"

Emulator::Emulator(int argc, char **argv, char **envp)
    : loader(argc, argv, envp), filename_(argv[1]) {}

Emulator::~Emulator() {}

int Emulator::init() {
  int err;
  printf("emu: start emulating\n");

  if ((err = loader.init()) != ESUCCESS) {
    printf("failed to loader initialization.\n");
    return err;
  }

  if ((err = loader.load()) != ESUCCESS) {
    printf("failed to load.\n");
    return err;
  }

  printf("loader.entry = 0x%lx\n", loader.entry);
  printf("loader.init_sp = 0x%lx\n", loader.init_sp);
  printf("loader.text_start = 0x%lx\n", loader.text_start);
  printf("loader.text_size = 0x%lx\n", loader.text_size);
  printf("loader.map_base = 0x%lx\n", loader.map_base);

  cpu.init(loader.entry, loader.init_sp, loader.text_start, loader.text_size,
           loader.map_base);

  init_done_ = true;
  return ESUCCESS;
}

void Emulator::execute_loop() {

  uint32_t inst;
  int i = 0;
  while (true) {
    inst = cpu.fetch();
    if (!inst) {
      break;
    }
    cpu.decode_start(inst);
    if (i >= 80000) {
      break;
    }
    i++;
  }
  cpu.show_regs();
  free((void *)loader.sp_alloc_start);
}
