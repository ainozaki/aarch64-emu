#pragma once

#include <cstdint>
#include <elf.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "const.h"

const uint64_t STACK_SIZE = 1000 * 1000;

class Loader {
public:
  uint64_t entry;
  uint64_t init_sp;
  uint64_t sp_alloc_start; // for free

  Loader(int argc, char **argv, char **envp);
  ~Loader();
  SysResult load();

private:
  int fd_; // for close
  char *filename_;
  int argc_;
  char **argv_;
  char **envp_;
  struct stat sb_;
  char *file_map_start_;
  Elf64_Ehdr *eh_;
  Elf64_Shdr *sh_tbl_;
  Elf64_Phdr *ph_tbl_;
  char *sh_name_;

  SysResult init();
  const char *get_interp() const;
  uint64_t get_map_total_size() const;
  uint64_t get_map_min_addr() const;
  uint64_t get_map_max_addr() const;
};