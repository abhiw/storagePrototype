#include "../include/tuple/tuple_header.h"

#include <gtest/gtest.h>

TEST(TupleHeaderTest, CreateHeader) {
  TupleHeader header(10, 2);
  EXPECT_EQ(header.GetFieldCount(), 10);
  EXPECT_EQ(header.GetVarFieldCount(), 2);
}

TEST(TupleHeaderTest, NullBitmapInitiallyAllFalse) {
  TupleHeader header(64, 0);
  for (uint16_t i = 0; i < 64; i++) {
    EXPECT_FALSE(header.IsFieldNull(i));
  }
}

TEST(TupleHeaderTest, SetFieldNullSingleBit) {
  TupleHeader header(10, 0);
  header.SetFieldNull(3, true);

  EXPECT_TRUE(header.IsFieldNull(3));
  EXPECT_FALSE(header.IsFieldNull(2));
  EXPECT_FALSE(header.IsFieldNull(4));
}

TEST(TupleHeaderTest, SetFieldNullMultipleBits) {
  TupleHeader header(10, 0);
  header.SetFieldNull(0, true);
  header.SetFieldNull(5, true);
  header.SetFieldNull(9, true);

  EXPECT_TRUE(header.IsFieldNull(0));
  EXPECT_FALSE(header.IsFieldNull(1));
  EXPECT_TRUE(header.IsFieldNull(5));
  EXPECT_FALSE(header.IsFieldNull(8));
  EXPECT_TRUE(header.IsFieldNull(9));
}

TEST(TupleHeaderTest, SetFieldNullClearBit) {
  TupleHeader header(10, 0);
  header.SetFieldNull(3, true);
  EXPECT_TRUE(header.IsFieldNull(3));

  header.SetFieldNull(3, false);
  EXPECT_FALSE(header.IsFieldNull(3));
}

TEST(TupleHeaderTest, SetFieldNullToggleBit) {
  TupleHeader header(10, 0);
  header.SetFieldNull(7, true);
  EXPECT_TRUE(header.IsFieldNull(7));

  header.SetFieldNull(7, false);
  EXPECT_FALSE(header.IsFieldNull(7));

  header.SetFieldNull(7, true);
  EXPECT_TRUE(header.IsFieldNull(7));
}

TEST(TupleHeaderTest, NullBitmapBoundaries) {
  TupleHeader header(64, 0);

  header.SetFieldNull(0, true);
  EXPECT_TRUE(header.IsFieldNull(0));

  header.SetFieldNull(31, true);
  EXPECT_TRUE(header.IsFieldNull(31));

  header.SetFieldNull(63, true);
  EXPECT_TRUE(header.IsFieldNull(63));

  EXPECT_FALSE(header.IsFieldNull(1));
  EXPECT_FALSE(header.IsFieldNull(30));
  EXPECT_FALSE(header.IsFieldNull(62));
}

TEST(TupleHeaderTest, VariableLengthOffsets) {
  TupleHeader header(10, 3);

  header.SetVariableLengthOffset(0, 100);
  header.SetVariableLengthOffset(1, 250);
  header.SetVariableLengthOffset(2, 500);

  EXPECT_EQ(header.GetVariableLengthOffset(0), 100);
  EXPECT_EQ(header.GetVariableLengthOffset(1), 250);
  EXPECT_EQ(header.GetVariableLengthOffset(2), 500);
}

TEST(TupleHeaderTest, VariableLengthOffsetsUpdate) {
  TupleHeader header(10, 2);

  header.SetVariableLengthOffset(0, 100);
  EXPECT_EQ(header.GetVariableLengthOffset(0), 100);

  header.SetVariableLengthOffset(0, 200);
  EXPECT_EQ(header.GetVariableLengthOffset(0), 200);
}

TEST(TupleHeaderTest, CalculateHeaderSizeNoVariableFields) {
  size_t size = TupleHeader::CalculateHeaderSize(0);
  EXPECT_EQ(size, 8);
}

TEST(TupleHeaderTest, CalculateHeaderSizeOneVariableField) {
  size_t size = TupleHeader::CalculateHeaderSize(1);
  EXPECT_EQ(size, 16);
}

TEST(TupleHeaderTest, CalculateHeaderSizeMultipleVariableFields) {
  EXPECT_EQ(TupleHeader::CalculateHeaderSize(2), 16);
  EXPECT_EQ(TupleHeader::CalculateHeaderSize(3), 16);
  EXPECT_EQ(TupleHeader::CalculateHeaderSize(4), 16);
  EXPECT_EQ(TupleHeader::CalculateHeaderSize(5), 24);
}

TEST(TupleHeaderTest, GetHeaderSize) {
  TupleHeader header1(10, 0);
  EXPECT_EQ(header1.GetHeaderSize(), 8);

  TupleHeader header2(10, 1);
  EXPECT_EQ(header2.GetHeaderSize(), 16);

  TupleHeader header3(10, 5);
  EXPECT_EQ(header3.GetHeaderSize(), 24);
}

TEST(TupleHeaderTest, SerializeDeserializeNoVariableFields) {
  TupleHeader header(10, 0);
  header.SetFieldNull(1, true);
  header.SetFieldNull(5, true);

  char buffer[8];
  header.SerializeTo(buffer);

  auto deserialized = TupleHeader::DeserializeFrom(buffer, 10, 0);
  EXPECT_EQ(deserialized.GetFieldCount(), 10);
  EXPECT_EQ(deserialized.GetVarFieldCount(), 0);
  EXPECT_TRUE(deserialized.IsFieldNull(1));
  EXPECT_TRUE(deserialized.IsFieldNull(5));
  EXPECT_FALSE(deserialized.IsFieldNull(0));
}

TEST(TupleHeaderTest, SerializeDeserializeWithVariableFields) {
  TupleHeader header(10, 3);
  header.SetFieldNull(2, true);
  header.SetFieldNull(7, true);
  header.SetVariableLengthOffset(0, 100);
  header.SetVariableLengthOffset(1, 250);
  header.SetVariableLengthOffset(2, 500);

  char buffer[16];
  header.SerializeTo(buffer);

  auto deserialized = TupleHeader::DeserializeFrom(buffer, 10, 3);
  EXPECT_EQ(deserialized.GetFieldCount(), 10);
  EXPECT_EQ(deserialized.GetVarFieldCount(), 3);
  EXPECT_TRUE(deserialized.IsFieldNull(2));
  EXPECT_TRUE(deserialized.IsFieldNull(7));
  EXPECT_FALSE(deserialized.IsFieldNull(0));
  EXPECT_EQ(deserialized.GetVariableLengthOffset(0), 100);
  EXPECT_EQ(deserialized.GetVariableLengthOffset(1), 250);
  EXPECT_EQ(deserialized.GetVariableLengthOffset(2), 500);
}

TEST(TupleHeaderTest, EightByteAlignment) {
  EXPECT_EQ(TupleHeader::CalculateHeaderSize(0) % 8, 0);
  EXPECT_EQ(TupleHeader::CalculateHeaderSize(1) % 8, 0);
  EXPECT_EQ(TupleHeader::CalculateHeaderSize(7) % 8, 0);
  EXPECT_EQ(TupleHeader::CalculateHeaderSize(10) % 8, 0);
  EXPECT_EQ(TupleHeader::CalculateHeaderSize(100) % 8, 0);
}

TEST(TupleHeaderTest, AllFieldsNull) {
  TupleHeader header(32, 0);
  for (uint16_t i = 0; i < 32; i++) {
    header.SetFieldNull(i, true);
  }

  for (uint16_t i = 0; i < 32; i++) {
    EXPECT_TRUE(header.IsFieldNull(i));
  }
}

TEST(TupleHeaderTest, AlternatingNullPattern) {
  TupleHeader header(16, 0);
  for (uint16_t i = 0; i < 16; i++) {
    header.SetFieldNull(i, i % 2 == 0);
  }

  for (uint16_t i = 0; i < 16; i++) {
    EXPECT_EQ(header.IsFieldNull(i), i % 2 == 0);
  }
}
