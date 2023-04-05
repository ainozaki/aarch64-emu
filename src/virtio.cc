#include "virtio.h"

#include <stdio.h>
#include <stdlib.h>

#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "log.h"
#include "utils.h"

void Virtio::store(uint64_t addr, uint64_t value) {
  switch (addr) {
  case VIRTIO_MMIO_MAGIC_VALUE:
  case VIRTIO_MMIO_VERSION:
  case VIRTIO_MMIO_DEVICE_ID:
  case VIRTIO_MMIO_VENDER_ID:
  case VIRTIO_MMIO_DEVICE_FEATURES:
  case VIRTIO_MMIO_QUEUE_NUM_MAX:
    // read only
    printf("err: virtio store to read-only register. 0x%lx\n", addr);
    assert(false);
    exit(1);
  case VIRTIO_MMIO_DRIVER_FEATURES:
    driver_features = value;
    LOG_CPU("virtio store VIRTIO_MMIO_DRIVER = 0x%lx\n", status);
    break;
  case VIRTIO_MMIO_GUEST_PAGE_SIZE:
    guest_page_size = value;
    LOG_CPU("virtio store VIRTIO_MMIO_GUEST_PAGE_SIZE = 0x%lx\n", status);
    break;
  case VIRTIO_MMIO_QUEUE_SEL:
    queue_sel = value;
    LOG_CPU("virtio store VIRTIO_MMIO_QUEUE_SEL = 0x%lx\n", status);
    break;
  case VIRTIO_MMIO_STATUS:
    status = value;
    LOG_CPU("virtio store VIRTIO_MMIO_STATUS = 0x%lx\n", status);
    break;
  default:
    printf("virtio store unsupported address 0x%lx\n", addr);
    exit(1);
  }
}

uint64_t Virtio::load(uint64_t addr) {
  switch (addr) {
  case VIRTIO_MMIO_DRIVER_FEATURES:
  case VIRTIO_MMIO_GUEST_PAGE_SIZE:
  case VIRTIO_MMIO_QUEUE_SEL:
    // write only
    printf("err: virtio load to write-only register. 0x%lx\n", addr);
    assert(false);
    exit(1);
  case VIRTIO_MMIO_MAGIC_VALUE:
    LOG_CPU("virtio VIRTIO_MMIO_MAGIC_VALUE = 0x%lx\n", magic_value);
    return magic_value;
  case VIRTIO_MMIO_VERSION:
    LOG_CPU("virtio VIRTIO_MMIO_VERSION = 0x%lx\n", version);
    return version;
  case VIRTIO_MMIO_DEVICE_ID:
    LOG_CPU("virtio VIRTIO_MMIO_DEVICE_ID = 0x%lx\n", device_id);
    return device_id;
  case VIRTIO_MMIO_VENDER_ID:
    LOG_CPU("virtio VIRTIO_MMIO_VENDER_ID = 0x%lx\n", vender_id);
    return vender_id;
  case VIRTIO_MMIO_DEVICE_FEATURES:
    LOG_CPU("virtio VIRTIO_MMIO_DEVICE_FEATURES = 0x%lx\n", device_features);
    return device_features;
  case VIRTIO_MMIO_QUEUE_NUM_MAX:
    LOG_CPU("virtio VIRTIO_MMIO_QUEUE_NUM_MAX = 0x%lx\n", queue_num_max);
    return queue_num_max;
  case VIRTIO_MMIO_STATUS:
    LOG_CPU("virtio VIRTIO_MMIO_STATUS = 0x%lx\n", status);
    return status;
  default:
    printf("virtio load unsupported address 0x%lx\n", addr);
    exit(1);
  }
}