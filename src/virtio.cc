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
    // read only
    LOG_CPU("err: virtio store to read-only register. 0x%lx\n", address);
    assert(false);
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
  case VIRTIO_MMIO_STATUS:
    LOG_CPU("virtio VIRTIO_MMIO_STATUS = 0x%lx\n", status);
    return status;
  default:
    printf("virtio load unsupported address 0x%lx\n", addr);
    exit(1);
  }
}