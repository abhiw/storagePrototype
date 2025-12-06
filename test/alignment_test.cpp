//
// Created by Abhi on 01/12/25.
//

#include "../include/schema/alignment.h"

#include <gtest/gtest.h>

// Test CalculateAlignment for all data types
TEST(AlignmentTest, CalculateAlignmentAllTypes) {
  // 1-byte alignment
  EXPECT_EQ(alignment::CalculateAlignment(BOOLEAN), 1);
  EXPECT_EQ(alignment::CalculateAlignment(TINYINT), 1);
  EXPECT_EQ(alignment::CalculateAlignment(CHAR), 1);

  // 2-byte alignment
  EXPECT_EQ(alignment::CalculateAlignment(SMALLINT), 2);

  // 4-byte alignment
  EXPECT_EQ(alignment::CalculateAlignment(INTEGER), 4);
  EXPECT_EQ(alignment::CalculateAlignment(FLOAT), 4);

  // 8-byte alignment
  EXPECT_EQ(alignment::CalculateAlignment(BIGINT), 8);
  EXPECT_EQ(alignment::CalculateAlignment(DOUBLE), 8);
}

// Test CalculatePadding with various scenarios
TEST(AlignmentTest, CalculatePaddingAlreadyAligned) {
  // When offset is already aligned, padding should be 0
  EXPECT_EQ(alignment::CalculatePadding(0, 1), 0);
  EXPECT_EQ(alignment::CalculatePadding(0, 2), 0);
  EXPECT_EQ(alignment::CalculatePadding(0, 4), 0);
  EXPECT_EQ(alignment::CalculatePadding(0, 8), 0);

  EXPECT_EQ(alignment::CalculatePadding(4, 4), 0);
  EXPECT_EQ(alignment::CalculatePadding(8, 4), 0);
  EXPECT_EQ(alignment::CalculatePadding(8, 8), 0);
  EXPECT_EQ(alignment::CalculatePadding(16, 8), 0);
}

TEST(AlignmentTest, CalculatePaddingNeedsAlignment) {
  // Test 4-byte alignment
  EXPECT_EQ(alignment::CalculatePadding(1, 4), 3);  // 1 -> 4 (need 3 bytes)
  EXPECT_EQ(alignment::CalculatePadding(2, 4), 2);  // 2 -> 4 (need 2 bytes)
  EXPECT_EQ(alignment::CalculatePadding(3, 4), 1);  // 3 -> 4 (need 1 byte)
  EXPECT_EQ(alignment::CalculatePadding(5, 4), 3);  // 5 -> 8 (need 3 bytes)

  // Test 8-byte alignment
  EXPECT_EQ(alignment::CalculatePadding(1, 8), 7);   // 1 -> 8 (need 7 bytes)
  EXPECT_EQ(alignment::CalculatePadding(5, 8), 3);   // 5 -> 8 (need 3 bytes)
  EXPECT_EQ(alignment::CalculatePadding(9, 8), 7);   // 9 -> 16 (need 7 bytes)
  EXPECT_EQ(alignment::CalculatePadding(10, 8), 6);  // 10 -> 16 (need 6 bytes)

  // Test 2-byte alignment
  EXPECT_EQ(alignment::CalculatePadding(1, 2), 1);  // 1 -> 2 (need 1 byte)
  EXPECT_EQ(alignment::CalculatePadding(3, 2), 1);  // 3 -> 4 (need 1 byte)
  EXPECT_EQ(alignment::CalculatePadding(5, 2), 1);  // 5 -> 6 (need 1 byte)
}

TEST(AlignmentTest, CalculatePaddingEdgeCases) {
  // Zero alignment (edge case)
  EXPECT_EQ(alignment::CalculatePadding(5, 0), 0);
  EXPECT_EQ(alignment::CalculatePadding(100, 0), 0);

  // 1-byte alignment (no padding ever needed)
  EXPECT_EQ(alignment::CalculatePadding(1, 1), 0);
  EXPECT_EQ(alignment::CalculatePadding(7, 1), 0);
  EXPECT_EQ(alignment::CalculatePadding(99, 1), 0);
}

// Test AlignOffset with different data types
TEST(AlignmentTest, AlignOffsetAlreadyAligned) {
  // Offset 0 is aligned for all types
  EXPECT_EQ(alignment::AlignOffset(0, BOOLEAN), 0);
  EXPECT_EQ(alignment::AlignOffset(0, SMALLINT), 0);
  EXPECT_EQ(alignment::AlignOffset(0, INTEGER), 0);
  EXPECT_EQ(alignment::AlignOffset(0, DOUBLE), 0);

  // Offset 8 is aligned for all types
  EXPECT_EQ(alignment::AlignOffset(8, BOOLEAN), 8);
  EXPECT_EQ(alignment::AlignOffset(8, SMALLINT), 8);
  EXPECT_EQ(alignment::AlignOffset(8, INTEGER), 8);
  EXPECT_EQ(alignment::AlignOffset(8, DOUBLE), 8);
}

TEST(AlignmentTest, AlignOffsetNeedsAlignment) {
  // BOOLEAN, TINYINT, CHAR (1-byte alignment) - never needs padding
  EXPECT_EQ(alignment::AlignOffset(5, BOOLEAN), 5);
  EXPECT_EQ(alignment::AlignOffset(7, TINYINT), 7);
  EXPECT_EQ(alignment::AlignOffset(13, CHAR), 13);

  // SMALLINT (2-byte alignment)
  EXPECT_EQ(alignment::AlignOffset(1, SMALLINT), 2);  // 1 -> 2
  EXPECT_EQ(alignment::AlignOffset(3, SMALLINT), 4);  // 3 -> 4
  EXPECT_EQ(alignment::AlignOffset(5, SMALLINT), 6);  // 5 -> 6

  // INTEGER, FLOAT (4-byte alignment)
  EXPECT_EQ(alignment::AlignOffset(1, INTEGER), 4);    // 1 -> 4
  EXPECT_EQ(alignment::AlignOffset(5, INTEGER), 8);    // 5 -> 8
  EXPECT_EQ(alignment::AlignOffset(10, INTEGER), 12);  // 10 -> 12
  EXPECT_EQ(alignment::AlignOffset(3, FLOAT), 4);      // 3 -> 4

  // BIGINT, DOUBLE (8-byte alignment)
  EXPECT_EQ(alignment::AlignOffset(1, BIGINT), 8);    // 1 -> 8
  EXPECT_EQ(alignment::AlignOffset(5, BIGINT), 8);    // 5 -> 8
  EXPECT_EQ(alignment::AlignOffset(9, BIGINT), 16);   // 9 -> 16
  EXPECT_EQ(alignment::AlignOffset(10, DOUBLE), 16);  // 10 -> 16
}

// Test realistic layout scenario
TEST(AlignmentTest, RealisticStructLayout) {
  // Simulating a struct layout:
  // struct Example {
  //   char a;      // 1 byte at offset 0
  //   int b;       // 4 bytes, needs 4-byte alignment
  //   char c;      // 1 byte
  //   double d;    // 8 bytes, needs 8-byte alignment
  // };

  size_t offset = 0;

  // char a at offset 0
  offset = alignment::AlignOffset(offset, CHAR);
  EXPECT_EQ(offset, 0);
  offset += 1;  // char is 1 byte, now offset = 1

  // int b needs 4-byte alignment
  offset = alignment::AlignOffset(offset, INTEGER);
  EXPECT_EQ(offset, 4);  // padded from 1 to 4
  offset += 4;           // int is 4 bytes, now offset = 8

  // char c at offset 8
  offset = alignment::AlignOffset(offset, CHAR);
  EXPECT_EQ(offset, 8);
  offset += 1;  // char is 1 byte, now offset = 9

  // double d needs 8-byte alignment
  offset = alignment::AlignOffset(offset, DOUBLE);
  EXPECT_EQ(offset, 16);  // padded from 9 to 16
}

// Test large offsets
TEST(AlignmentTest, LargeOffsets) {
  EXPECT_EQ(alignment::AlignOffset(1000, INTEGER), 1000);  // already aligned
  EXPECT_EQ(alignment::AlignOffset(1001, INTEGER), 1004);  // 1001 -> 1004
  EXPECT_EQ(alignment::AlignOffset(1000, DOUBLE), 1000);   // already aligned
  EXPECT_EQ(alignment::AlignOffset(1005, DOUBLE), 1008);   // 1005 -> 1008
}
