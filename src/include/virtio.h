#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <stdint.h>

#include "const.h"

class Cpu;

const uint64_t VIRTIO_MMIO = 0xa000000;
const uint64_t VIRTIO_MMIO_MAGIC_VALUE = VIRTIO_MMIO;
const uint64_t VIRTIO_MMIO_VERSION = VIRTIO_MMIO + 0x4;
const uint64_t VIRTIO_MMIO_DEVICE_ID = VIRTIO_MMIO + 0x8;
const uint64_t VIRTIO_MMIO_VENDER_ID = VIRTIO_MMIO + 0xc;
const uint64_t VIRTIO_MMIO_HOST_FEATURES = VIRTIO_MMIO + 0x10;
const uint64_t VIRTIO_MMIO_HOST_FEATURES_SEL = VIRTIO_MMIO + 0x14;
const uint64_t VIRTIO_MMIO_GUEST_FEATURES = VIRTIO_MMIO + 0x20;
const uint64_t VIRTIO_MMIO_GUEST_FEATURES_SEL = VIRTIO_MMIO + 0x24;
const uint64_t VIRTIO_MMIO_GUEST_PAGE_SIZE = VIRTIO_MMIO + 0x28;
const uint64_t VIRTIO_MMIO_QUEUE_SEL = VIRTIO_MMIO + 0x30;
const uint32_t VIRTIO_MMIO_QUEUE_NUM_MAX = VIRTIO_MMIO + 0x34;
const uint32_t VIRTIO_MMIO_QUEUE_NUM = VIRTIO_MMIO + 0x38;
const uint32_t VIRTIO_MMIO_QUEUE_ALIGN = VIRTIO_MMIO + 0x3c;
const uint32_t VIRTIO_MMIO_QUEUE_PFN = VIRTIO_MMIO + 0x40;
const uint32_t VIRTIO_MMIO_QUEUE_NOTIFY = VIRTIO_MMIO + 0x50;
const uint32_t VIRTIO_MMIO_INTERRUPT_STATUS = VIRTIO_MMIO + 0x60;
const uint32_t VIRTIO_MMIO_INTERRUPT_ACK = VIRTIO_MMIO + 0x64;
const uint64_t VIRTIO_MMIO_STATUS = VIRTIO_MMIO + 0x70;
const uint64_t VIRTIO_MMIO_CONFIG = VIRTIO_MMIO + 0x100;

const uint64_t VRING_DESC_F_NEXT = 0x1;
const uint64_t VRING_DESC_SIZE = 0x10;

// See
// https://docs.oasis-open.org/virtio/virtio/v1.1/csprd01/virtio-v1.1-csprd01.html#x1-1560004
struct virtio_mmio_control_registers {
  uint32_t magic_value = 0x74726976;

  // Version ID
  // 1 for legacy device.
  uint32_t version = 1;

  // Device ID
  // 2 for block device.
  uint32_t device_id = 2;

  uint32_t vender_id = 0x554d4551;

  // Flags representing features the device supports
  // If HostFearutesSel & 0x1 = 0, returns bits 0 to 31.
  // If HostFearutesSel & 0x1 = 1, returns bits 32 to 63.
  uint32_t host_features = 0x31006ed4;

  // Device features word selection
  uint32_t host_heatures_sel;

  // Flags representing features understood and activated by the driver
  // Same as host_features.
  uint32_t guest_features;

  // Activated gurest features word selection
  uint32_t guest_features_sel;

  // Guest page size
  // This value is writtewn by the driver during initialization,
  // and used by the device to calculate the Guest address of the first queue
  // page.
  uint64_t guest_page_size = 0;

  // Virtual queue index
  // Selects the virtual queue that QueueNumMax, QueueNum, QueueAlign and
  // QueuePFN registers apply to.
  uint32_t queue_sel;

  // Maximum virtual queue size
  // Maximum size of the queue the device is ready to process.
  // This value applies to the "QUEUE_SEL"-selected queue, and only when
  // QueuePFN == 0.
  uint32_t queue_num_max = 0x10;

  // Virtual queue size
  // The number of elements in the queue.
  // Writing to this register notifies the device what size the driver will use.
  uint32_t queue_num = 0;

  // Used Ring alignment in the virtual queue
  // Writing to this register notifies the device about alignment boundary of
  // the Used ring. Should be power of 2/
  uint32_t queue_align;

  // Guest physical page number of the virtual queue
  // The location of the virtual queue in the Guest's physical address space.
  // This value is the index number of a page starting with the queue Desctiptor
  // Table. Zero means that the driver stops using the queue.
  uint32_t queue_pfn = 0;

  // Queue notifier
  // Writin to this register notifies the device that there are new buffers to
  // process in a queue.
  uint32_t queue_notify = UINT32_MAX;

  // Interrupt status
  // This is a read-only register representing interrupt caused by the device.
  // bit 0: Used Buffer Notification
  // bit 1: Configuration Change Notification
  uint32_t interrupt_status;
  uint32_t interrupt_ack;

  // Device status
  // Writing zero means reset.
  uint32_t status = 0;
};

struct avairable_ring {
  uint16_t flags;
  uint16_t idx;
  uint16_t *rings;
};

class Desc {
  public:
    Desc(uint64_t base_addr, Cpu *cpu);
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};

class Virtqueue {
public:
  Virtqueue(uint32_t pfn, uint32_t page_size, uint32_t queue_num);
  ~Virtqueue() = default;

  void update(uint32_t pfn, uint32_t page_size, uint32_t queue_num);

  // Descriptor Table
  // size is 16 * queue_num
  // u64 addr
  // u32 len
  // u16 flags
  // u16 next
  uint64_t desc_table;

  // Guest -> Host 
  // size is 6 + 2 * queue_num
  // u16 flags
  // u16 idx
  // u16[QUEUE_NUM] rings
  // u16 	used_event
  uint64_t avail_ring;
  uint16_t avail_idx = 0;

  // Host -> Guest
  // size is 6 + 8 * queue_num
  // u16 flags
  // u16 idx
  // UsedRingEntry[QUEUE_NUM] rings
  //    u32 id
  //    u32 len
  // u16 avail_event
  uint64_t used_ring;
};

class Virtio {
public:
  Virtio() = default;
  ~Virtio() = default;

  void store(uint64_t addr, uint64_t value);
  uint64_t load(uint64_t addr);

  void disk_access(Cpu *cpu);

  bool is_interrupting();

private:
  struct virtio_mmio_control_registers control_regs;
  std::vector<uint8_t> disk;
  std::optional<Virtqueue> virtqueue;
};
