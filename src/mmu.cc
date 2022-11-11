#include "mmu.h"

#include <cassert>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"

void MMU::init(Bus *bus){
  assert(bus);
  bus_ = bus;
}

uint64_t MMU::mmu_translate(uint64_t addr) {
  //    vaddr
  //     63   39 38    30 29    21 20    12 11       0
  //    +-------+--------+--------+--------+----------+
  //    | TTBRn | level1 | level2 | level3 | page off |
  //    |       | (PUD)  | (PMD)  | (PTE)  |          |
  //    +-------+--------+--------+--------+----------+
  //
  uint32_t ttbrn;
  uint16_t pud, pmd, pte, offset;

  if (!if_mmu_enabled()) {
    return addr;
  }
  
  
  if ((addr >= ram_base) && (addr <= ram_base + ram_size)){
    return addr;
  }else { 
    uint64_t mapped_addr = addr - 0xffffff8000000000;
    return mapped_addr;
  }

  ttbrn = util::shift(addr, 39, 63);
  pud = util::shift(addr, 30, 38);
  pmd = util::shift(addr, 21, 29);
  pte = util::shift(addr, 12, 20);
  offset = util::shift(addr, 0, 11);
  printf("======================\n");
  printf("addr   = 0x%lx\n", addr);
  printf("ttbrn   = 0x%x\n", ttbrn);
  printf("L1(pud) = 0x%x\n", pud);
  printf("L2(pmd) = 0x%x\n", pmd);
  printf("L3(pte) = 0x%x\n", pte);
  printf("offset  = 0x%x\n", offset);
  printf("======================\n");
  
  uint64_t base_addr, index, value, paddr;
  if (ttbrn == 0) {
    // L0
    base_addr = ttbr0_el1;
    index = pud * 8;
    value = bus_->load((uint64_t)(base_addr + index), MemAccessSize::DWord);
    printf("L1");
    printf("\tbase_addr   = 0x%lx\n", base_addr);
    printf("\t*0x%lx = 0x%lx\n", base_addr + index ,value);
  
    // L2
    base_addr = value;
    index = pmd * 8;
    value = bus_->load((uint64_t)(base_addr + index), MemAccessSize::DWord);
    printf("L2");
    printf("\tbase_addr   = 0x%lx\n", base_addr);
    printf("\t*0x%lx = 0x%lx\n", base_addr + index ,value);
    
    // L3
    base_addr = value;
    index = pte * 8;
    value = bus_->load((uint64_t)(base_addr + index), MemAccessSize::DWord);
    printf("L3");
    printf("\tbase_addr   = 0x%lx\n", base_addr);
    printf("\t*0x%lx = 0x%lx\n", base_addr + index ,value);
  } else {
    printf("ttbr1_el1 = 0x%lx\n", ttbr1_el1);
  }
  paddr = (util::shift(value, 12, 38) << 12) | offset;
  printf("paddr = 0x%lx\n", paddr);
  printf("======================\n");
  exit(0);
  return addr;
}