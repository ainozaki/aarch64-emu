#include "loader.h"

#include <algorithm>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "log.h"
#include "utils.h"

Loader::Loader(int argc, char **argv, char **envp)
    : filename_(argv[1]), argc_(argc), argv_(argv), envp_(envp) {}

Loader::~Loader() {
  close(fd_);
  munmap(file_map_start_, sb_.st_size);
}

int Loader::init() {
  LOG_SYSTEM("Loading %s\n", filename_);

  // open ELF file
  fd_ = open(filename_, O_RDWR);
  if (!fd_) {
    LOG_SYSTEM("Cannot open %s\n", filename_);
    return -1;
  }
  fstat(fd_, &sb_);

  // mmap ELF file
  if ((file_map_start_ = (char *)mmap(NULL, (sb_.st_size + 4095) & ~4095UL,
                                      PROT_READ | PROT_WRITE, MAP_PRIVATE, fd_,
                                      0)) == (char *)-1) {
    perror("mmap");
    return -1;
  }
  LOG_SYSTEM("\tfile_map_start: %p, size:%zx -> %zx\n", file_map_start_,
             sb_.st_size, (sb_.st_size + 4095) & ~4095UL);

  // parse ELF file
  eh_ = (Elf64_Ehdr *)file_map_start_;
  ph_tbl_ = (Elf64_Phdr *)((uint64_t)file_map_start_ + eh_->e_phoff);
  sh_tbl_ = (Elf64_Shdr *)((uint64_t)file_map_start_ + eh_->e_shoff);
  sh_name_ =
      (char *)((uint64_t)file_map_start_ + sh_tbl_[eh_->e_shstrndx].sh_addr);
  return 0;
}

int Loader::load() {
  // TODO: ELF MAGIC check

  // interp check
  // This emulator can load only static linked binary.
  const char *interp_str = get_interp();
  if (interp_str) {
    LOG_SYSTEM("cannot load dynamic linked program\n");
    return -1;
  }

  // allocate RAM for emulator and 0 clear
  void *ram_base;
  if ((ram_base = mmap(NULL, RAM_SIZE, PROT_READ | PROT_EXEC | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (void *)-1) {
    perror("mmap");
    return -1;
  }
  memset(ram_base, 0, RAM_SIZE);
  map_base = (uint64_t)ram_base;
  LOG_SYSTEM("\tmap_base(host):0x%lx, RAM_SIZE:0x%x\n", map_base, RAM_SIZE);

  LOG_SYSTEM("\ttext_start_paddr:0x%lx\n", text_start_paddr);

  // load segment
  Elf64_Phdr *ph;
  for (int i = 0; i < eh_->e_phnum; i++) {
    ph = &ph_tbl_[i];

    if (ph->p_type != PT_LOAD) {
      continue;
    }

    if (ph->p_vaddr != ph->p_paddr) {
      use_paddr_ = true;
    }

    // memcpy LOAD segment
    LOG_SYSTEM("\tmemcpy:\n");
    LOG_SYSTEM("map_base : 0x%lx\n", map_base);
    LOG_SYSTEM("\t\tload (emu): 0x%lx-0x%lx, size:0x%lx\n", ph->p_paddr,
               ph->p_paddr + ph->p_memsz, ph->p_memsz);
    LOG_SYSTEM("\t\tmemcpy: dst:0x%lx, size:0x%lx\n",
               map_base + ph->p_paddr - text_start_paddr, ph->p_memsz);
    LOG_SYSTEM(
        "\t\tmemcpy: src: file_map_start:%p, ph->p_offset:0x%lx, size:0x%lx\n",
        file_map_start_, ph->p_offset, ph->p_memsz);
    memcpy((void *)(map_base + ph->p_paddr - text_start_paddr),
           (void *)((uint64_t)file_map_start_ + ph->p_offset), ph->p_filesz);

    // zero clear .bss section
    if (ph->p_memsz > ph->p_filesz) {
      LOG_SYSTEM("\t\tzeroclear .bss (host):from:0x%lx, size:0x%lx\n",
                 map_base + ph->p_paddr + ph->p_filesz - text_start_paddr,
                 ph->p_memsz - ph->p_filesz);
      LOG_SYSTEM("\t\tzeroclear .bss (emu) :from:0x%lx, size:0x%lx\n",
                 ph->p_paddr + ph->p_filesz, ph->p_memsz - ph->p_filesz);
      memset((void *)(map_base + ph->p_paddr + ph->p_filesz - text_start_paddr),
             0, ph->p_memsz - ph->p_filesz);
    }
    entry = eh_->e_entry - ph->p_vaddr + ph->p_paddr;
  }

  // prepare stack
  init_sp = text_start_paddr + RAM_SIZE;
  init_sp = init_sp - (init_sp % 16) + 16;
  LOG_SYSTEM("\tinit_sp: 0x%lx\n", init_sp);
  return 0;
}

const char *Loader::get_interp() const {
  for (int i = 0; i < eh_->e_phnum; i++) {
    if (ph_tbl_[i].p_type == PT_INTERP) {
      return file_map_start_ + ph_tbl_[i].p_offset;
    }
  }
  return NULL;
}