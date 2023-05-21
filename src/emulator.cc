#include "emulator.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <thread>
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

void Emulator::log_pc(uint64_t pc, const char *msg, uint64_t idx) {
  if (cpu->pc == pc) {
    LOG_SYSTEM("##### %ld %s: pc:0x%lx, sp:0x%lx, x7:0x%lx\n", idx, msg,
               cpu->pc, cpu->sp, cpu->xregs[7]);
  }
}

void read_stdin(uint8_t *uart_rx_buff, uint8_t *uart_rx_idx) {
  while (1) {
    uint8_t idx = (*uart_rx_idx) % UART_RX_BUFF_LEN;
    char *buff = (char *)uart_rx_buff + idx;
    if (!fgets(buff, 10, stdin)) {
      break;
    }
    for (uint8_t i = 0; i < 10; i++) {
      *uart_rx_idx = *uart_rx_idx + 1;
      if (*(buff + i) == '\n') {
        break;
      }
    }
  }
}

void Emulator::execute_loop() {

  // FILE *flog;
  // flog = fopen("sp_debug_emu.txt", "w");

  uint32_t inst;
  int i = 0;

  std::thread read_stdin_thread(read_stdin, cpu->bus.uart.uart_rx_buff,
                                &cpu->bus.uart.uart_rx_idx);

  while (true) {
    // log_cpu_on = 0;
    cpu->timer_count += 1;
    cpu->check_interrupt();

    inst = cpu->fetch();
    if (!inst) {
      LOG_SYSTEM("no instructions 0x%lx\n", cpu->pc);
      break;
    }
    /*
    log_pc(0xffffff80400060e4, "el0sync", i);
    log_pc(0xffffff8040006198, "el0irq", i);
    log_pc(0xffffff8040005f84, "el1sync", i);
    log_pc(0xffffff8040006034, "el1irq", i);
    log_pc(0xffffff80400054d0, "switchuvm", i);
    log_pc(0xffffff8040001f3c, "sched", i);
    log_pc(0xffffff8040001ea0, "scheduler", i);
    log_pc(0xffffff8040001da0, "fork", i);
    log_pc(0xffffff80400048b0, "exec", i);
    log_pc(0xffffff8040001450, "switchuvm", i);
    log_pc(0xffffff804000253c, "swtch", i);
    log_pc(0xffffff8040006298, "timerintr", i);
    log_pc(0xffffff8040001bf8, "allocproc", i);
    log_pc(0xffffff8040000dec, "safestrcpy", i);
    log_pc(0xffffff8040006248, "eret", i);
    log_pc(0xffffff8040005800, "alltraps", i);
    log_pc(0xffffff8040001da0, "fork", i);
    log_pc(0xffffff8040005c00, "svc", i);
    log_pc(0xffffff80400029b0, "syscall", i);
    */

    /*
        log_pc(0xffffff80400007c4,"uartinit", i);
        log_pc(0xffffff8040006254,"timerinit", i);
        log_pc(0xffffff8040002c14,"binit", i);
        log_pc(0xffffff80400031cc,"fsinit", i);
        log_pc(0xffffff8040004134,"fileinit", i);
        log_pc(0xffffff8040000b48,"initlock", i);
        log_pc(0xffffff8040000f84,"kvminithart", i);
        log_pc(0xffffff80400013e0,"uvminit", i);
        log_pc(0xffffff8040016578,"initproc", i);
        log_pc(0xffffff804000630c,"virtio_disk_init", i);
        log_pc(0xffffff8040002594,"trapinit", i);
        log_pc(0xffffff804000688c,"gicv3inithart", i);
        log_pc(0xffffff8040009298,"initcode", i);
        log_pc(0xffffff8040002594,"trapinithart", i);
        log_pc(0xffffff80400012cc,"kvminit", i);
        log_pc(0xffffff8040000ab0,"kinit1", i);
        log_pc(0xffffff804000078c,"printfinit", i);
        log_pc(0xffffff8040001cb4,"userinit", i);
        log_pc(0xffffff80400067ac,"gicv3init", i);
        log_pc(0xffffff8040000430,"consoleinit", i);
        log_pc(0xffffff804000400c,"initsleeplock", i);
        log_pc(0xffffff8040003244,"iinit", i);
        log_pc(0xffffff8040003d30,"initlog", i);
        log_pc(0xffffff8040001a68,"procinit", i);
        log_pc(0xffffff8040000aec,"kinit2", i);
        */

    log_pc(0xffffff8040004d68, "argfd", i);
    log_pc(0xffffff804000292c, "argint", i);
    log_pc(0xffffff8040002954, "argaddr", i);
    log_pc(0xffffff8040004350, "fileread", i);

    log_pc(0xffffff8040000500, "panic", i);
    // log_pc(0xffffff80400009f8, "kfree", i);
    log_pc(0xffffff8040004ecc, "sys_close", i);
    log_pc(0xffffff8040002ab0, "sys_sbrk", i);
    log_pc(0xffffff8040002a64, "sys_fork", i);
    log_pc(0xffffff8040005370, "sys_mkdir", i);
    log_pc(0xffffff8040004e1c, "sys_read", i);
    log_pc(0xffffff8040002b90, "sys_kill", i);
    log_pc(0xffffff8040004f10, "sys_fstat", i);
    log_pc(0xffffff8040002af8, "sys_sleep", i);
    log_pc(0xffffff8040004f54, "sys_link", i);
    log_pc(0xffffff8040005218, "sys_open", i);
    log_pc(0xffffff8040004e74, "sys_write", i);
    log_pc(0xffffff8040004dd0, "sys_dup", i);
    log_pc(0xffffff804000506c, "sys_unlink", i);
    log_pc(0xffffff8040002bc4, "sys_uptime", i);
    log_pc(0xffffff80400054c8, "sys_exec", i);
    log_pc(0xffffff8040002a7c, "sys_wait", i);
    log_pc(0xffffff8040002a18, "sys_exit", i);
    log_pc(0xffffff80400055c8, "sys_pipe", i);
    log_pc(0xffffff8040002a4c, "sys_getpid", i);
    log_pc(0xffffff8040005440, "sys_chdir", i);
    log_pc(0xffffff80400053c8, "sys_mknod", i);

    log_pc(0xffffff80400160f8, "uart_tx_buf", i);
    log_pc(0xffffff804000086c, "uartstart", i);
    log_pc(0xffffff80400160e8, "uart_tx_r", i);
    log_pc(0xffffff80400160f0, "uart_tx_w", i);
    log_pc(0xffffff804000099c, "uartintr", i);
    log_pc(0xffffff8040000970, "uartgetc", i);
    log_pc(0xffffff80400160d0, "uart_tx_lock", i);
    log_pc(0xffffff80400008ec, "uartputc", i);
    log_pc(0xffffff804000081c, "uartputc_sync", i);

    log_pc(0xffffff80400001e8, "consoleread", i);
    log_pc(0xffffff8040000174, "consolewrite", i);
    log_pc(0xffffff8040000318, "consoleintr", i);
    log_pc(0xffffff8040000430, "consoleinit", i);

    log_pc(0xd8, "getcmd", i);
    log_pc(0x17c, "runcmd", i);
    log_pc(0xae4, "parsecmd", i);
    log_pc(0x8e0, "parseline", i);
    log_pc(0x5d4, "peek", i);

    if (cpu->pc == 0x5f4) {
      // log_cpu_on = 1;
    }

    if (log_cpu_on) {
      printf("=== %d 0x%lx ", i, cpu->pc);
    }

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
