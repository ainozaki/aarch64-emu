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

SysResult Emulator::init() {
  SysResult err;
  printf("emu: start emulating\n");

  err = loader.load();
  if (err != SysResult::Success) {
    return err;
  }

  cpu.init(loader.entry, loader.init_sp, loader.text_start, loader.text_size,
           loader.map_base);

  init_done_ = true;
  return SysResult::Success;
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
    if (i >= 100000) {
      break;
    }
    i++;
  }
  cpu.show_regs();
  free((void *)loader.sp_alloc_start);
}
