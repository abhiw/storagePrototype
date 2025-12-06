//
// Created by Abhi on 29/11/25.
//

#include "../../include/common/checksum.h"

// Static member definitions
uint32_t checksum::lookup_table_[256];
std::once_flag checksum::init_flag_;

void checksum::BuildLookupTable() {
  for (int i = 0; i < 256; i++) {
    uint32_t crc_value = static_cast<uint32_t>(i) << 24;
    for (int j = 0; j < 8; j++) {
      if (crc_value & 0x80000000) {
        crc_value = (crc_value << 1) ^ POLYNOMIAL;
      } else {
        crc_value = crc_value << 1;
      }
    }
    lookup_table_[i] = crc_value;
  }
}

void checksum::EnsureTableInitialized() {
  std::call_once(init_flag_, BuildLookupTable);
}

uint32_t checksum::Compute(const uint8_t* data, const std::size_t length) {
  uint32_t result_crc = Init();
  result_crc = Update(result_crc, data, length);
  return Finalize(result_crc);
}

uint32_t checksum::Init() {
  EnsureTableInitialized();
  return INITIAL_CRC;
}

uint32_t checksum::Update(uint32_t crc, const uint8_t* data,
                          const std::size_t length) {
  EnsureTableInitialized();

  for (size_t i = 0; i < length; i++) {
    const uint8_t table_index = ((crc >> 24) ^ data[i]) & 0xFF;
    crc = (crc << 8) ^ lookup_table_[table_index];
  }

  return crc;
}

uint32_t checksum::Finalize(const uint32_t crc) { return ~crc; }