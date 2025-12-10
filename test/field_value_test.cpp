#include "../include/tuple/field_value.h"

#include <gtest/gtest.h>

#include <limits>

TEST(FieldValueTest, CreateBoolean) {
  auto fv = FieldValue::Boolean(true);
  EXPECT_FALSE(fv.IsNull());
  EXPECT_EQ(fv.GetType(), DataType::BOOLEAN);
  EXPECT_TRUE(fv.GetBoolean());

  auto fv2 = FieldValue::Boolean(false);
  EXPECT_FALSE(fv2.GetBoolean());
}

TEST(FieldValueTest, CreateTinyInt) {
  auto fv = FieldValue::TinyInt(42);
  EXPECT_FALSE(fv.IsNull());
  EXPECT_EQ(fv.GetType(), DataType::TINYINT);
  EXPECT_EQ(fv.GetTinyInt(), 42);

  auto fv_neg = FieldValue::TinyInt(-128);
  EXPECT_EQ(fv_neg.GetTinyInt(), -128);
}

TEST(FieldValueTest, CreateSmallInt) {
  auto fv = FieldValue::SmallInt(1000);
  EXPECT_FALSE(fv.IsNull());
  EXPECT_EQ(fv.GetType(), DataType::SMALLINT);
  EXPECT_EQ(fv.GetSmallInt(), 1000);

  auto fv_min = FieldValue::SmallInt(std::numeric_limits<int16_t>::min());
  EXPECT_EQ(fv_min.GetSmallInt(), std::numeric_limits<int16_t>::min());
}

TEST(FieldValueTest, CreateInteger) {
  auto fv = FieldValue::Integer(100000);
  EXPECT_FALSE(fv.IsNull());
  EXPECT_EQ(fv.GetType(), DataType::INTEGER);
  EXPECT_EQ(fv.GetInteger(), 100000);

  auto fv_max = FieldValue::Integer(std::numeric_limits<int32_t>::max());
  EXPECT_EQ(fv_max.GetInteger(), std::numeric_limits<int32_t>::max());
}

TEST(FieldValueTest, CreateBigInt) {
  auto fv = FieldValue::BigInt(9223372036854775807LL);
  EXPECT_FALSE(fv.IsNull());
  EXPECT_EQ(fv.GetType(), DataType::BIGINT);
  EXPECT_EQ(fv.GetBigInt(), 9223372036854775807LL);
}

TEST(FieldValueTest, CreateFloat) {
  auto fv = FieldValue::Float(3.14f);
  EXPECT_FALSE(fv.IsNull());
  EXPECT_EQ(fv.GetType(), DataType::FLOAT);
  EXPECT_FLOAT_EQ(fv.GetFloat(), 3.14f);

  auto fv_neg = FieldValue::Float(-0.0f);
  EXPECT_FLOAT_EQ(fv_neg.GetFloat(), -0.0f);
}

TEST(FieldValueTest, CreateDouble) {
  auto fv = FieldValue::Double(2.718281828);
  EXPECT_FALSE(fv.IsNull());
  EXPECT_EQ(fv.GetType(), DataType::DOUBLE);
  EXPECT_DOUBLE_EQ(fv.GetDouble(), 2.718281828);
}

TEST(FieldValueTest, CreateChar) {
  auto fv = FieldValue::Char("A");
  EXPECT_FALSE(fv.IsNull());
  EXPECT_EQ(fv.GetType(), DataType::CHAR);
  EXPECT_EQ(fv.GetString(), "A");
}

TEST(FieldValueTest, CreateVarChar) {
  auto fv = FieldValue::VarChar("Hello, World!");
  EXPECT_FALSE(fv.IsNull());
  EXPECT_EQ(fv.GetType(), DataType::VARCHAR);
  EXPECT_EQ(fv.GetString(), "Hello, World!");

  auto fv_empty = FieldValue::VarChar("");
  EXPECT_EQ(fv_empty.GetString(), "");
}

TEST(FieldValueTest, CreateText) {
  std::string long_text(1000, 'x');
  auto fv = FieldValue::Text(long_text);
  EXPECT_FALSE(fv.IsNull());
  EXPECT_EQ(fv.GetType(), DataType::TEXT);
  EXPECT_EQ(fv.GetString(), long_text);
}

TEST(FieldValueTest, CreateBlob) {
  std::vector<uint8_t> blob_data = {0x00, 0xFF, 0xAB, 0xCD};
  auto fv = FieldValue::Blob(blob_data);
  EXPECT_FALSE(fv.IsNull());
  EXPECT_EQ(fv.GetType(), DataType::BLOB);
  EXPECT_EQ(fv.GetBlob(), blob_data);

  std::vector<uint8_t> empty_blob;
  auto fv_empty = FieldValue::Blob(empty_blob);
  EXPECT_EQ(fv_empty.GetBlob().size(), 0);
}

TEST(FieldValueTest, CreateNull) {
  auto fv = FieldValue::Null(DataType::INTEGER);
  EXPECT_TRUE(fv.IsNull());
  EXPECT_EQ(fv.GetType(), DataType::INTEGER);
  EXPECT_THROW(fv.GetInteger(), std::runtime_error);

  auto fv_varchar = FieldValue::Null(DataType::VARCHAR);
  EXPECT_TRUE(fv_varchar.IsNull());
  EXPECT_THROW(fv_varchar.GetString(), std::runtime_error);
}

TEST(FieldValueTest, TypeMismatchBoolean) {
  auto fv = FieldValue::Integer(42);
  EXPECT_THROW(fv.GetBoolean(), std::runtime_error);
}

TEST(FieldValueTest, TypeMismatchInteger) {
  auto fv = FieldValue::Double(3.14);
  EXPECT_THROW(fv.GetInteger(), std::runtime_error);
}

TEST(FieldValueTest, TypeMismatchString) {
  auto fv = FieldValue::Integer(100);
  EXPECT_THROW(fv.GetString(), std::runtime_error);
}

TEST(FieldValueTest, TypeMismatchBlob) {
  auto fv = FieldValue::VarChar("test");
  EXPECT_THROW(fv.GetBlob(), std::runtime_error);
}

TEST(FieldValueTest, SerializedSizeFixedTypes) {
  EXPECT_EQ(FieldValue::Boolean(true).GetSerializedSize(), 1);
  EXPECT_EQ(FieldValue::TinyInt(42).GetSerializedSize(), 1);
  EXPECT_EQ(FieldValue::SmallInt(1000).GetSerializedSize(), 2);
  EXPECT_EQ(FieldValue::Integer(100000).GetSerializedSize(), 4);
  EXPECT_EQ(FieldValue::BigInt(1000000000LL).GetSerializedSize(), 8);
  EXPECT_EQ(FieldValue::Float(3.14f).GetSerializedSize(), 4);
  EXPECT_EQ(FieldValue::Double(2.718).GetSerializedSize(), 8);
}

TEST(FieldValueTest, SerializedSizeVariableTypes) {
  auto fv_varchar = FieldValue::VarChar("Hello");
  EXPECT_EQ(fv_varchar.GetSerializedSize(), sizeof(uint16_t) + 5);

  auto fv_text = FieldValue::Text("Test");
  EXPECT_EQ(fv_text.GetSerializedSize(), sizeof(uint16_t) + 4);

  std::vector<uint8_t> blob_data(100);
  auto fv_blob = FieldValue::Blob(blob_data);
  EXPECT_EQ(fv_blob.GetSerializedSize(), sizeof(uint16_t) + 100);
}

TEST(FieldValueTest, SerializedSizeNull) {
  auto fv_null = FieldValue::Null(DataType::INTEGER);
  EXPECT_EQ(fv_null.GetSerializedSize(), 0);

  auto fv_null_varchar = FieldValue::Null(DataType::VARCHAR);
  EXPECT_EQ(fv_null_varchar.GetSerializedSize(), 0);
}

TEST(FieldValueTest, BoundaryValues) {
  auto fv_int_min = FieldValue::Integer(std::numeric_limits<int32_t>::min());
  EXPECT_EQ(fv_int_min.GetInteger(), std::numeric_limits<int32_t>::min());

  auto fv_int_max = FieldValue::Integer(std::numeric_limits<int32_t>::max());
  EXPECT_EQ(fv_int_max.GetInteger(), std::numeric_limits<int32_t>::max());

  auto fv_bigint_min = FieldValue::BigInt(std::numeric_limits<int64_t>::min());
  EXPECT_EQ(fv_bigint_min.GetBigInt(), std::numeric_limits<int64_t>::min());

  auto fv_bigint_max = FieldValue::BigInt(std::numeric_limits<int64_t>::max());
  EXPECT_EQ(fv_bigint_max.GetBigInt(), std::numeric_limits<int64_t>::max());
}
