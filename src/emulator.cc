#include "emulator.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "cpu.h"
#include "loader.h"
#include "log.h"
#include "mem.h"
#include "utils.h"

int log_system_on = 0;
int log_cpu_on = 0;
int log_debug_on = 0;

Emulator::Emulator(int argc, char **argv, char **envp, const std::string &disk)
    : loader(argc, argv, envp), filename_(argv[1]) {

  // LOG_SYSTEM("emu: start emulating\n");

  if (loader.init() != ESUCCESS) {
    // LOG_SYSTEM("failed to loader initialization.\n");
    return;
  }

  if (loader.load() != ESUCCESS) {
    // LOG_SYSTEM("failed to load.\n");
    return;
  }

  // LOG_SYSTEM("loader.entry = 0x%lx\n", loader.entry);
  // LOG_SYSTEM("loader.init_sp = 0x%lx\n", loader.init_sp);
  // LOG_SYSTEM("loader.text_start = 0x%lx\n", loader.text_start_paddr);
  //  //LOG_SYSTEM("loader.text_size = 0x%lx\n", loader.text_size);
  // LOG_SYSTEM("loader.map_base = 0x%lx\n", loader.map_base);

  cpu = std::make_unique<Cpu>(loader.entry, loader.init_sp,
                              loader.text_start_paddr, loader.text_size,
                              loader.map_base, disk);

  init_done_ = true;
  return;
}

void Emulator::log_pc(uint64_t pc, const char *msg) {
  if (cpu->pc == pc) {
    LOG_SYSTEM("##### %s: pc:0x%lx, sp:0x%lx, x7:0x%lx\n", msg, cpu->pc,
               cpu->sp, cpu->xregs[7]);
  }
}

void Emulator::execute_loop() {

  FILE *flog;
  flog = fopen("sp_debug_emu.txt", "w");

  uint32_t inst;
  int i = 0;
  // int num_insts = 20000000;
  //  int num_insts = 67905;
  //  int num_insts = 67670;
  int debugidx = 0;

  while (true) {
    cpu->timer_count += 1;
    cpu->check_interrupt();

    inst = cpu->fetch();
    if (!inst) {
      LOG_SYSTEM("no instructions 0x%lx\n", cpu->pc);
      break;
    }
    log_pc(0xffffff80400060e4, "el0sync");
    log_pc(0xffffff8040006198, "el0irq");
    log_pc(0xffffff8040005f84, "el1sync");
    log_pc(0xffffff8040006034, "el1irq");
    log_pc(0xffffff80400054d0, "switchuvm");
    log_pc(0xffffff8040001f3c, "sched");
    log_pc(0xffffff8040001ea0, "scheduler");
    log_pc(0xffffff80400054d0, "sys_exec");
    log_pc(0xffffff80400029b0, "syscall");
    log_pc(0xffffff8040002a6c, "sys_fork");
    log_pc(0xffffff8040001da0, "fork");
    log_pc(0xffffff8040002a20, "sys_exit");
    log_pc(0xffffff8040005220, "sys_open");
    log_pc(0xffffff80400048b0, "exec");
    log_pc(0xffffff8040001450, "switchuvm");
    log_pc(0xffffff804000253c, "swtch");
    log_pc(0xffffff8040006298, "timerintr");
    log_pc(0xffffff8040001bf8, "allocproc");
    log_pc(0xffffff8040000dec, "safestrcpy");
    log_pc(0xffffff8040005c00, "svc");
    log_pc(0xffffff8040006248, "eret");
    log_pc(0xffffff8040005800, "alltraps");
    log_pc(0xffffff8040001da0, "fork");
    log_pc(0xffffff8040001ca8, "userinit");

    if (cpu->pc == 0xffffff8040000500) {
      // LOG_SYSTEM("panic\n");
      // sleep(10);
    }
    if (log_cpu_on) {
      LOG_SYSTEM("===%d 0x%lx ", i, cpu->pc);
      ////LOG_SYSTEM("=== %d 0x%lx 0x%lx\n", i, cpu->pc,
      /// cpu->load(0xffffff80400054d0,MemAccessSize::Word)); /LOG_SYSTEM("===
      /// %d 0x%lx ", i, cpu->pc);
      // f//LOG_SYSTEM(flog, "### 0x%lx 0x%lx 0x%lx\n", cpu->pc, cpu->sp,
      // cpu->xregs[7]); LOG_SYSTEM("0x%lx 0x%lx 0x%lx, SP_EL1=0x%lx\n",
      // cpu->pc, cpu->sp, cpu->xregs[7], cpu->SP_EL1);
      /*
      //LOG_SYSTEM("inst 0x%x\n", inst);
      //LOG_SYSTEM("pc 0x%lx\n", cpu->pc);
      // //LOG_SYSTEM("sp=0x%lx:\n", sp);
      // //LOG_SYSTEM("0x%lx: \t", pc);
      //LOG_SYSTEM("sp 0x%lx\n", cpu->sp);
      ////LOG_SYSTEM("cpsr 0x%x\n", cpsr);
      //LOG_SYSTEM("x0 0x%lx\n", cpu->xregs[0]);
      //LOG_SYSTEM("x1 0x%lx\n", cpu->xregs[1]);
      //LOG_SYSTEM("x2 0x%lx\n", cpu->xregs[2]);
      //LOG_SYSTEM("x3 0x%lx\n", cpu->xregs[3]);
      //LOG_SYSTEM("x19 0x%lx\n", cpu->xregs[19]);
      //LOG_SYSTEM("x20 0x%lx\n", cpu->xregs[20]);
      //LOG_SYSTEM("x29 0x%lx\n", cpu->xregs[29]);
      //LOG_SYSTEM("x30 0x%lx\n", cpu->xregs[30]);
      */
    }
    /*
    if (debugidx > 1000){
      debug = false;
    }
    // f//LOG_SYSTEM(stderr, "%d 0x%lx 0x%lx 0x%lx  0x%lx 0x%lx 0x%lx 0x%lx\n",
    i,
    // cpu.pc, cpu.sp, cpu.xregs[0], cpu.xregs[1], cpu.xregs[19], cpu.xregs[29],
    // cpu.xregs[30]);
    */
    cpu->decode_start(inst);
    i++;
  }
  // cpu.show_regs();
  munmap((void *)loader.map_base, RAM_SIZE);
}

int main(int argc, char **argv, char **envp) {
  if (argc < 2) {
    // LOG_SYSTEM("usage: %s <filename>\n", argv[0]);
    return 0;
  }

  const std::string diskname = "fs.img";
  Emulator emu(argc, argv, envp, diskname);
  if (emu.init_done_) {
    emu.execute_loop();
  }
  return 0;
}
