#pragma once

#include <cstdint>

#include "const.h"

const uint64_t VIRTIO_MMIO = 0xa000000;
const uint64_t VIRTIO_MMIO_MAGIC_VALUE = VIRTIO_MMIO;
const uint64_t VIRTIO_MMIO_VERSION = VIRTIO_MMIO + 0x4;
const uint64_t VIRTIO_MMIO_DEVICE_ID = VIRTIO_MMIO + 0x8;
const uint64_t VIRTIO_MMIO_VENDER_ID = VIRTIO_MMIO + 0xc;
const uint64_t VIRTIO_MMIO_DEVICE_FEATURES = VIRTIO_MMIO + 0x10;
const uint64_t VIRTIO_MMIO_STATUS = VIRTIO_MMIO + 0x70;

class Virtio {
public:
  Virtio() = default;
  ~Virtio() = default;

  void store(uint64_t addr, uint64_t value);
  uint64_t load(uint64_t addr);

private:
  uint32_t magic_value = 0x74726976;
  uint32_t version = 1;
  uint32_t device_id = 2; // device
  uint32_t vender_id = 0x554d4551;
  uint32_t device_features;
  uint32_t status;
};