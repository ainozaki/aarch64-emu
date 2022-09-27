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

Emulator::~Emulator()
{
  // free
  free((void *)loader.sp_alloc_start);
  // ummap?
}

SysResult Emulator::init()
{
  SysResult err;
  printf("emu: start emulating\n");

  err = loader.load();
  if (err != SysResult::Success)
  {
    return err;
  }
  cpu.pc = loader.entry;
  cpu.xregs[31] = loader.init_sp;

  return SysResult::Success;
}

void Emulator::execute_loop()
{
  /*
  uint32_t inst;
  while (true)
  {
    inst = cpu.fetch();
    cpu.decode_start(inst);
  }
  */
  cpu.show_regs();
}