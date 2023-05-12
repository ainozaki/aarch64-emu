#include "virtio.h"

#include <stdio.h>
#include <stdlib.h>

#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

#include "cpu.h"
#include "log.h"
#include "utils.h"

Virtio::Virtio(const std::string &diskname) {
  std::ifstream fin(diskname, std::ios::binary);
  if (!fin) {
    std::cerr << "Cannot open file " << diskname << std::endl;
    return;
  }

  fin.seekg(0, std::ios::end);
  std::streamsize fsize = fin.tellg();
  fin.seekg(0, std::ios::beg);
  fin.unsetf(std::ios::skipws);
  disk.insert(disk.begin(), std::istream_iterator<uint8_t>(fin),
              std::istream_iterator<uint8_t>());
  
  fin.close();
  printf("disk size: 0x%lx, loaded: 0x%lx\n", fsize, disk.size());
}

Desc::Desc(uint64_t base_addr, Cpu *cpu) {
  addr = cpu->bus.mem.load64(base_addr);
  len = cpu->bus.mem.load32(base_addr + 8);
  flags = cpu->bus.mem.load16(base_addr + 12);
  next = cpu->bus.mem.load16(base_addr + 14);
}

Virtqueue::Virtqueue(uint32_t pfn, uint32_t page_size, uint32_t queue_num) {
  update(pfn, page_size, queue_num);
}

void Virtqueue::update(uint32_t pfn, uint32_t page_size, uint32_t queue_num) {
  uint64_t base = pfn * page_size;
  // Virtqueue part, Alignment, Size
  // Descriptor Table, 16, 16*(Queue Size)
  // Available Ring, 2, 6 + 2*(Queue Size)
  // Used Ring, 4, 6 + 8*(Queue Size)
  desc_table = base;
  avail_ring = base + queue_num * 16;
  //used_ring = avail_ring + 6 + queue_num * 2;
  used_ring = base + page_size;
  printf("\tvirtqueue: desc=0x%lx, avail=0x%lx, used=0x%lx\n", desc_table,
         avail_ring, used_ring);
  printf("\tvirtio.pfn = 0x%x\n", pfn);
  printf("\tvirtio.page_size = 0x%x\n", page_size);
  printf("\tvirtio.queue_num = 0x%x\n", queue_num);
}

bool Virtio::is_interrupting() {
  if (control_regs.queue_notify == 0) {
    control_regs.queue_notify = UINT32_MAX;
    return true;
  }
  return false;
}

void Virtio::disk_access(Cpu *cpu) {

  uint64_t first_index =
      cpu->bus.mem.load64(virtqueue->avail_ring + 32 +
                          virtqueue->avail_idx % control_regs.queue_num);
  printf("first index = %ld\n", first_index);

  Desc desc0 = Desc(virtqueue->desc_table + first_index * VRING_DESC_SIZE, cpu);
  Desc desc1 = Desc(virtqueue->desc_table + desc0.next * VRING_DESC_SIZE, cpu);
  Desc desc2 = Desc(virtqueue->desc_table + desc1.next * VRING_DESC_SIZE, cpu);
  printf("desc0: addr=0x%lx, len=0x%x, flags=%x, next=0x%x\n", desc0.addr,
         desc0.len, desc0.flags, desc0.next);
  printf("desc1: addr=0x%lx, len=0x%x, flags=%x, next=0x%x\n", desc1.addr,
         desc1.len, desc1.flags, desc1.next);
  printf("desc2: addr=0x%lx, len=0x%x, flags=%x, next=0x%x\n", desc2.addr,
         desc2.len, desc2.flags, desc2.next);

  assert(desc0.flags & VRING_DESC_F_NEXT == 1);
  assert(desc1.flags & VRING_DESC_F_NEXT == 1);
  assert(desc2.flags & VRING_DESC_F_NEXT == 0);

  // struct virtio_blk_req {
  //   le32 type;
  //   le32 reserved;
  //   le64 sector;
  //   u8 data[][512];
  //   u8 status;
  // };
  uint64_t sector = cpu->bus.mem.load64(desc0.addr + 8);
  if (desc1.flags & VRING_DESC_F_WRITE) {
    // driver read, device write
    for (uint16_t i = 0; i < desc1.len; i++) {
      uint8_t data = disk[sector * SECTOR_SIZE + i];
      cpu->bus.mem.store8(desc1.addr + i, data);
    }
  } else {
    // driver write, device read
    for (uint16_t i = 0; i < desc1.len; i++) {
      uint8_t data = cpu->bus.mem.load8(desc1.addr + i);
      disk[sector * SECTOR_SIZE + i] = data;
    }
  }
  // Notification
  cpu->bus.mem.store8(desc2.addr, 0);
  cpu->bus.mem.store32(virtqueue->used_ring + 4 +
                           (id % control_regs.queue_num) * 8,
                       first_index);
  id += 1;
  cpu->bus.mem.store16(virtqueue->used_ring + 2, id);
  printf("used.idx = 0x%x\n", cpu->bus.mem.load16(virtqueue->used_ring + 2));
}

void Virtio::store(uint64_t addr, uint64_t value) {
  switch (addr) {
  case VIRTIO_MMIO_MAGIC_VALUE:
  case VIRTIO_MMIO_VERSION:
  case VIRTIO_MMIO_DEVICE_ID:
  case VIRTIO_MMIO_VENDER_ID:
  case VIRTIO_MMIO_HOST_FEATURES:
  case VIRTIO_MMIO_QUEUE_NUM_MAX:
    // read only
    printf("err: virtio store to read-only register. 0x%lx\n", addr);
    assert(false);
    exit(1);
  case VIRTIO_MMIO_GUEST_FEATURES:
    control_regs.guest_features = value;
    printf("virtio store VIRTIO_MMIO_DRIVER = 0x%x\n",
           control_regs.guest_features);
    break;
  case VIRTIO_MMIO_GUEST_PAGE_SIZE:
    control_regs.guest_page_size = value;
    printf("virtio store VIRTIO_MMIO_GUEST_PAGE_SIZE = 0x%lx\n",
           control_regs.guest_page_size);
    virtqueue->update(control_regs.queue_pfn, control_regs.guest_page_size,
                      control_regs.queue_num);
    break;
  case VIRTIO_MMIO_QUEUE_SEL:
    control_regs.queue_sel = value;
    printf("virtio store VIRTIO_MMIO_QUEUE_SEL = 0x%x\n",
           control_regs.queue_sel);
    break;
  case VIRTIO_MMIO_QUEUE_NUM:
    control_regs.queue_num = value;
    printf("virtio store VIRTIO_MMIO_QUEUE_NUM = 0x%x\n",
           control_regs.queue_num);
    virtqueue->update(control_regs.queue_pfn, control_regs.guest_page_size,
                      control_regs.queue_num);
    break;
  case VIRTIO_MMIO_QUEUE_PFN:
    control_regs.queue_pfn = value;
    printf("virtio store VIRTIO_MMIO_QUEUE_PFN = 0x%x\n",
           control_regs.queue_pfn);
    virtqueue->update(control_regs.queue_pfn, control_regs.guest_page_size,
                      control_regs.queue_num);
    break;
  case VIRTIO_MMIO_QUEUE_NOTIFY:
    control_regs.queue_notify = value;
    printf("virtio store VIRTIO_MMIO_QUEUE_NOTIFY = 0x%x\n",
           control_regs.queue_notify);
    break;
  case VIRTIO_MMIO_INTERRUPT_ACK:
    control_regs.interrupt_ack = value;
    printf("virtio store VIRTIO_MMIO_INTERRUPT_ACK = 0x%x\n",
           control_regs.interrupt_ack);
    break;
  case VIRTIO_MMIO_STATUS:
    control_regs.status = value;
    if (control_regs.status & 0x4) {
      virtqueue =
          Virtqueue(control_regs.queue_pfn, control_regs.guest_page_size,
                    control_regs.queue_num);
    }
    printf("virtio store VIRTIO_MMIO_STATUS = 0x%x\n", control_regs.status);
    break;
  default:
    printf("virtio store unsupported address 0x%lx\n", addr);
    exit(1);
  }
}

uint64_t Virtio::load(uint64_t addr) {
  switch (addr) {
  case VIRTIO_MMIO_GUEST_FEATURES:
  case VIRTIO_MMIO_GUEST_PAGE_SIZE:
  case VIRTIO_MMIO_QUEUE_SEL:
  case VIRTIO_MMIO_QUEUE_NUM:
  case VIRTIO_MMIO_QUEUE_NOTIFY:
    // write only
    printf("err: virtio load to write-only register. 0x%lx\n", addr);
    assert(false);
    exit(1);
  case VIRTIO_MMIO_MAGIC_VALUE:
    printf("virtio VIRTIO_MMIO_MAGIC_VALUE = 0x%x\n", control_regs.magic_value);
    return control_regs.magic_value;
  case VIRTIO_MMIO_VERSION:
    printf("virtio VIRTIO_MMIO_VERSION = 0x%x\n", control_regs.version);
    return control_regs.version;
  case VIRTIO_MMIO_DEVICE_ID:
    printf("virtio VIRTIO_MMIO_DEVICE_ID = 0x%x\n", control_regs.device_id);
    return control_regs.device_id;
  case VIRTIO_MMIO_VENDER_ID:
    printf("virtio VIRTIO_MMIO_VENDER_ID = 0x%x\n", control_regs.vender_id);
    return control_regs.vender_id;
  case VIRTIO_MMIO_HOST_FEATURES:
    printf("virtio VIRTIO_MMIO_DEVICE_FEATURES = 0x%x\n",
           control_regs.host_features);
    return control_regs.host_features;
  case VIRTIO_MMIO_QUEUE_NUM_MAX:
    printf("virtio VIRTIO_MMIO_QUEUE_NUM_MAX = 0x%x\n",
           control_regs.queue_num_max);
    return control_regs.queue_num_max;
  case VIRTIO_MMIO_QUEUE_PFN:
    printf("virtio VIRTIO_MMIO_QUEUE_PFN = 0x%x\n", control_regs.queue_pfn);
    return control_regs.queue_pfn;
  case VIRTIO_MMIO_INTERRUPT_ACK:
    printf("virtio VIRTIO_MMIO_INTERRUPT_ACK = 0x%x\n", control_regs.interrupt_ack);
    return control_regs.interrupt_ack;
  case VIRTIO_MMIO_INTERRUPT_STATUS:
    printf("virtio VIRTIO_MMIO_INTERRUPT_STATUS = 0x%x\n", control_regs.interrupt_status);
    return control_regs.interrupt_status;
  case VIRTIO_MMIO_STATUS:
    printf("virtio VIRTIO_MMIO_STATUS = 0x%x\n", control_regs.status);
    return control_regs.status;
  default:
    printf("virtio load unsupported address 0x%lx\n", addr);
    exit(1);
  }
}