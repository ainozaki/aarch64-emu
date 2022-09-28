#include "loader.h"

#include <algorithm>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

Loader::Loader(int argc, char **argv, char **envp)
    : filename_(argv[1]), argc_(argc), argv_(argv), envp_(envp) {}

Loader::~Loader() {
  close(fd_);
  munmap(file_map_start_, sb_.st_size);
}

namespace {
const uint64_t PAGE_SIZE = sysconf(_SC_PAGESIZE);

uint64_t PAGE_ROUNDDOWN(uint64_t v) { return v & ~(PAGE_SIZE - 1); }

uint64_t PAGE_ROUNDUP(uint64_t v) {
  return (v + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}
} // namespace

SysResult Loader::init() {
  printf("Loading %s\n", filename_);
  fd_ = open(filename_, O_RDWR);
  if (!fd_) {
    fprintf(stderr, "Cannot open %s\n", filename_);
    return SysResult::ErrorElf;
  }
  fstat(fd_, &sb_);
  if ((file_map_start_ = (char *)mmap(NULL, sb_.st_size, PROT_READ | PROT_WRITE,
                                      MAP_PRIVATE, fd_, 0)) == (char *)-1) {
    perror("mmap");
    return SysResult::ErrorElf;
  }
  printf("file_map_start: %p\n", file_map_start_);
  eh_ = (Elf64_Ehdr *)file_map_start_;
  ph_tbl_ = (Elf64_Phdr *)((uint64_t)file_map_start_ + eh_->e_phoff);
  sh_tbl_ = (Elf64_Shdr *)((uint64_t)file_map_start_ + eh_->e_shoff);
  sh_name_ =
      (char *)((uint64_t)file_map_start_ + sh_tbl_[eh_->e_shstrndx].sh_addr);
  return SysResult::Success;
}

SysResult Loader::load() {
  bool map_done;
  SysResult err;

  // map
  err = init();
  if (err != SysResult::Success) {
    return err;
  }

  // ELF check

  // interp check
  const char *interp_str = get_interp();
  if (interp_str) {
    printf("cannot load dynamic linked program\n");
    return SysResult::ErrorElf;
  }

  // map
  Elf64_Phdr *ph;
  for (int i = 0; i < eh_->e_phnum; i++) {
    ph = &ph_tbl_[i];

    if (ph->p_type != PT_LOAD) {
      continue;
    }

    if (!map_done) {
      uint64_t map_min_addr = get_map_min_addr();
      uint64_t map_max_addr = get_map_max_addr();
      uint64_t map_start = PAGE_ROUNDDOWN(map_min_addr);
      uint64_t map_size = PAGE_ROUNDUP(map_max_addr - map_start);
      uint64_t map_result;
      printf("\tmap: min=0x%lx, max=0x%lx, start=0x%lx, size=0x%lx\n",
             map_min_addr, map_max_addr, map_start, map_size);
      if ((map_result = (uint64_t)mmap((void *)map_min_addr, map_size,
                                       PROT_READ | PROT_EXEC | PROT_WRITE,
                                       MAP_SHARED | MAP_ANONYMOUS, 0, 0)) ==
          (uint64_t)-1) {
        perror("mmap");
        return SysResult::ErrorElf;
      }
      printf("\tmap_returned: 0x%lx\n", map_result);
      map_done = true;
    }

    printf("\tmemcpy: range:0x%lx-0x%lx, size:0x%lx\n", ph->p_vaddr,
           ph->p_vaddr + ph->p_memsz, ph->p_memsz);
    memcpy((void *)(ph->p_vaddr),
           (void *)((uint64_t)file_map_start_ + ph->p_offset), ph->p_memsz);

    if (ph->p_memsz > ph->p_filesz) {
      memset((void *)(ph->p_vaddr + ph->p_filesz), 0,
             ph->p_memsz - ph->p_filesz);
      printf("\tzero clear .bss: from:0x%lx, size:0x%lx\n",
             ph->p_vaddr + ph->p_filesz, ph->p_memsz - ph->p_filesz);
    }
  }

  // stack
  uint64_t stack_tp = (uint64_t)calloc(STACK_SIZE, sizeof(char *));
  sp_alloc_start = stack_tp;
  stack_tp = stack_tp - (stack_tp % 16) + 16;
  init_sp = stack_tp + STACK_SIZE - 16 * 4 * 200;
  printf("stack: 0x%lx - 0x%lx, init_sp: 0x%lx\n", stack_tp,
         stack_tp + STACK_SIZE, init_sp);
  uint64_t *sp = (uint64_t *)init_sp;
  uint8_t index = 0;
  // argc, argv
  sp[index++] = argc_ - 1;
  for (int i = 1; i < argc_; i++) {
    sp[index++] = (uint64_t)argv_[i];
  }
  sp[index++] = 0;
  // envp
  int envc = 0;
  while (envp_[envc]) {
    printf("\t%p: envp: 0x%lx\n", &sp[index], (uint64_t)envp_[envc]);
    sp[index++] = (uint64_t)envp_[envc++];
  }
  // auxv
  uint64_t *av = (uint64_t *)envp_ + envc + 1;
  while (*av) {
    uint64_t *type = av++;
    uint64_t *value = av++;
    if (type == NULL) {
      printf("null\n");
      *type = 0;
    }
    switch (*type) {
    case AT_BASE:
      *value = 0;
      break;
    case AT_EXECFD:
      *value = fd_;
      break;
    case AT_PHDR:
      *value = ph_tbl_[0].p_vaddr + eh_->e_phoff;
      break;
    case AT_PHNUM:
      *value = eh_->e_phnum;
      break;
    case AT_PHENT:
      *value = eh_->e_phentsize;
      break;
    case AT_ENTRY:
      *value = eh_->e_entry;
      break;
    case AT_NULL:
      printf("null\n");
    }
    printf("\t%p: auxv: type[0x%2lx]=0x%8lx\n", &sp[index], *type, *value);
    sp[index++] = *type;
    sp[index++] = *value;
  }

  entry = eh_->e_entry;
  return SysResult::Success;
}

const char *Loader::get_interp() const {
  for (int i = 0; i < eh_->e_phnum; i++) {
    if (ph_tbl_[i].p_type == PT_INTERP) {
      return file_map_start_ + ph_tbl_[i].p_offset;
    }
  }
  return NULL;
}

uint64_t Loader::get_map_total_size() const {
  uint64_t min_addr = 0;
  uint64_t max_addr = 0;
  for (int i = 0; i < eh_->e_phnum; i++) {
    if (ph_tbl_[i].p_type == PT_LOAD) {
      min_addr = std::min(ph_tbl_[i].p_vaddr, min_addr);
      max_addr = std::max(ph_tbl_[i].p_vaddr + ph_tbl_[i].p_memsz, max_addr);
    }
  }
  return max_addr - min_addr;
}
uint64_t Loader::get_map_max_addr() const {
  uint64_t max_addr = 0;
  for (int i = 0; i < eh_->e_phnum; i++) {
    if (ph_tbl_[i].p_type == PT_LOAD) {
      max_addr = std::max(ph_tbl_[i].p_vaddr + ph_tbl_[i].p_memsz, max_addr);
    }
  }
  return max_addr;
}

uint64_t Loader::get_map_min_addr() const {
  uint64_t min_addr = (uint64_t)-1;
  for (int i = 0; i < eh_->e_phnum; i++) {
    if (ph_tbl_[i].p_type == PT_LOAD) {
      min_addr = std::min(ph_tbl_[i].p_vaddr, min_addr);
    }
  }
  return min_addr;
}