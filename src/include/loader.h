#pragma once

#include <cstdint>
#include <elf.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const uint64_t STACK_SIZE = 1000 * 1000;
const uint32_t RAM_SIZE = 128 * 1024 * 1024; // 128MB

class Loader {
public:
  uint64_t entry;
  uint64_t init_sp;
  uint64_t map_base;
  uint64_t text_size;
  uint64_t sp_alloc_start; // for free
  const uint64_t text_start_paddr = 0x40000000;

  Loader(int argc, char **argv, char **envp);
  ~Loader();

  int init();
  int load();

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
  bool use_paddr_;

  const char *get_interp() const;
};
