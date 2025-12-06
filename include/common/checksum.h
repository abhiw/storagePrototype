//
// Created by Abhi on 29/11/25.
//

#ifndef STORAGEENGINE_CHECKSUM_H
#define STORAGEENGINE_CHECKSUM_H

#include <mutex>

class checksum final {
 public:
  static constexpr uint32_t POLYNOMIAL = 0x04C11DB7;
  static constexpr uint32_t INITIAL_CRC = 0xFFFFFFFF;

  static uint32_t Compute(const uint8_t* data, std::size_t length);

  // Incremental CRC32 computation
  static uint32_t Init();
  static uint32_t Update(uint32_t crc, const uint8_t* data, std::size_t length);
  static uint32_t Finalize(uint32_t crc);

 private:
  static uint32_t lookup_table_[256];
  static std::once_flag init_flag_;

  static void BuildLookupTable();
  static void EnsureTableInitialized();
};

#endif  // STORAGEENGINE_CHECKSUM_H
