//
// Created by Abhi on 29/11/25.
//

#include "../include/common/checksum.h"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

// Test correctness with known CRC32 values
TEST(ChecksumTest, KnownValues) {
  struct TestCase {
    std::string input;
    uint32_t expected_crc;
  };

  // CRC32 (MSB-first, polynomial 0x04C11DB7) known values
  std::vector<TestCase> test_cases = {
      {"", 0x00000000},
      {"a", 0x19939B6B},
      {"abc", 0x648CBB73},
      {"message digest", 0xBFC90357},
      {"abcdefghijklmnopqrstuvwxyz", 0x77BF9396},
      {"The quick brown fox jumps over the lazy dog", 0x459DEE61}};

  for (const auto& test_case : test_cases) {
    uint32_t result_crc = checksum::Compute(
        reinterpret_cast<const uint8_t*>(test_case.input.c_str()),
        test_case.input.size());
    EXPECT_EQ(result_crc, test_case.expected_crc)
        << "CRC mismatch for input: '" << test_case.input << "'";
  }
}

// Test thread safety with actual concurrent threads
TEST(ChecksumTest, ThreadSafety) {
  const std::string input = "The quick brown fox jumps over the lazy dog";
  const uint32_t expected = 0x459DEE61;

  std::vector<std::thread> threads;
  std::atomic<bool> failed{false};

  for (int i = 0; i < 10; i++) {
    threads.emplace_back([&]() {
      for (int j = 0; j < 100; j++) {
        uint32_t crc = checksum::Compute(
            reinterpret_cast<const uint8_t*>(input.c_str()), input.size());
        if (crc != expected) {
          failed = true;
        }
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_FALSE(failed) << "Thread safety issue detected";
}

// Test edge cases
TEST(ChecksumTest, EdgeCases) {
  // Empty data
  EXPECT_EQ(checksum::Compute(nullptr, 0), 0x00000000);

  // Single byte
  uint8_t byte = 'X';
  EXPECT_NE(checksum::Compute(&byte, 1), 0);

  // Large data
  std::vector<uint8_t> large_data(10000, 0xAB);
  EXPECT_NO_THROW(checksum::Compute(large_data.data(), large_data.size()));
}

// Test consistency - same input should always produce same output
TEST(ChecksumTest, Consistency) {
  const std::string input = "test data for consistency";
  uint32_t first_crc = checksum::Compute(
      reinterpret_cast<const uint8_t*>(input.c_str()), input.size());

  for (int i = 0; i < 10; i++) {
    uint32_t result_crc = checksum::Compute(
        reinterpret_cast<const uint8_t*>(input.c_str()), input.size());
    EXPECT_EQ(result_crc, first_crc) << "Inconsistent CRC on iteration " << i;
  }
}
