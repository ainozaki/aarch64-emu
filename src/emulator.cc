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

  LOG_SYSTEM("emu: start emulating\n");

  // Load ELF file
  if (loader.init() < 0) {
    LOG_SYSTEM("failed to loader initialization.\n");
    return;
  }
  if (loader.load() < 0) {
    LOG_SYSTEM("failed to load.\n");
    return;
  }

  LOG_SYSTEM("loader.entry = 0x%lx\n", loader.entry);
  LOG_SYSTEM("loader.init_sp = 0x%lx\n", loader.init_sp);
  LOG_SYSTEM("loader.text_start = 0x%lx\n", loader.text_start_paddr);
  LOG_SYSTEM("loader.text_size = 0x%lx\n", loader.text_size);
  LOG_SYSTEM("loader.map_base = 0x%lx\n", loader.map_base);

  // Create CPU
  cpu = std::make_unique<Cpu>(loader.entry, loader.init_sp,
                              loader.text_start_paddr, loader.text_size,
                              loader.map_base, disk);

  init_done_ = true;
  return;
}


// This function is executed in another thread for uart input
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

void Emulator::log_pc(uint64_t pc, const char *msg, uint64_t idx) {
  if (cpu->pc == pc) {
    LOG_SYSTEM("##### %ld %s: pc:0x%lx, sp:0x%lx, x7:0x%lx\n", idx, msg,
               cpu->pc, cpu->sp, cpu->xregs[7]);
  }
}

void Emulator::execute_loop() {
  uint32_t inst;
  int i = 0;

  std::thread read_stdin_thread(read_stdin, cpu->bus.uart.uart_rx_buff,
                                &cpu->bus.uart.uart_rx_idx);

  while (true) {
    cpu->timer_count += 1;
    cpu->check_interrupt();

    inst = cpu->fetch();
    if (!inst) {
      LOG_SYSTEM("no instructions 0x%lx\n", cpu->pc);
      break;
    }

    cpu->decode_start(inst);
    i++;

    /*

    debug
    log_pc(0xffffff8040000500, "panic", i);
    log_pc(0xffffff80400009f8, "kfree", i);
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
    if (log_cpu_on) {
      printf("=== %d 0x%lx ", i, cpu->pc);
    }
    */
  }
  munmap((void *)loader.map_base, RAM_SIZE);
}

int main(int argc, char **argv, char **envp) {
  if (argc < 2) {
    LOG_SYSTEM("usage: %s <kernel filename>\n", argv[0]);
    return 0;
  }

  const std::string diskname = "fs.img";

  Emulator emu(argc, argv, envp, diskname);
  if (emu.init_done_) {
    emu.execute_loop();
  }
  return 0;
}
