#include "../include/tuple/tuple_serializer.h"

#include <gtest/gtest.h>

TEST(TupleSerializerTest, SerializeFixedLengthAllTypes) {
  Schema schema;
  schema.AddColumn("col_bool", DataType::BOOLEAN, false, 0);
  schema.AddColumn("col_tinyint", DataType::TINYINT, false, 0);
  schema.AddColumn("col_smallint", DataType::SMALLINT, false, 0);
  schema.AddColumn("col_int", DataType::INTEGER, false, 0);
  schema.AddColumn("col_bigint", DataType::BIGINT, false, 0);
  schema.AddColumn("col_float", DataType::FLOAT, false, 0);
  schema.AddColumn("col_double", DataType::DOUBLE, false, 0);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Boolean(true));
  values.push_back(FieldValue::TinyInt(42));
  values.push_back(FieldValue::SmallInt(1000));
  values.push_back(FieldValue::Integer(100000));
  values.push_back(FieldValue::BigInt(1000000000LL));
  values.push_back(FieldValue::Float(3.14f));
  values.push_back(FieldValue::Double(2.718));

  char buffer[256];
  size_t size = TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                      sizeof(buffer));

  EXPECT_GT(size, 0);
  EXPECT_LE(size, sizeof(buffer));
}

TEST(TupleSerializerTest, RoundTripFixedLength) {
  Schema schema;
  schema.AddColumn("col_int", DataType::INTEGER, false, 0);
  schema.AddColumn("col_double", DataType::DOUBLE, false, 0);
  schema.Finalize();

  std::vector<FieldValue> original;
  original.push_back(FieldValue::Integer(12345));
  original.push_back(FieldValue::Double(98.6));

  char buffer[256];
  size_t size = TupleSerializer::SerializeFixedLength(schema, original, buffer,
                                                      sizeof(buffer));

  auto deserialized =
      TupleSerializer::DeserializeFixedLength(schema, buffer, size);

  ASSERT_EQ(deserialized.size(), 2);
  EXPECT_EQ(deserialized[0].GetInteger(), 12345);
  EXPECT_DOUBLE_EQ(deserialized[1].GetDouble(), 98.6);
}

TEST(TupleSerializerTest, FixedLengthWithNulls) {
  Schema schema;
  schema.AddColumn("col1", DataType::INTEGER, true, 0);
  schema.AddColumn("col2", DataType::DOUBLE, true, 0);
  schema.AddColumn("col3", DataType::SMALLINT, true, 0);
  schema.Finalize();

  std::vector<FieldValue> original;
  original.push_back(FieldValue::Null(DataType::INTEGER));
  original.push_back(FieldValue::Double(3.14));
  original.push_back(FieldValue::Null(DataType::SMALLINT));

  char buffer[256];
  size_t size = TupleSerializer::SerializeFixedLength(schema, original, buffer,
                                                      sizeof(buffer));

  auto deserialized =
      TupleSerializer::DeserializeFixedLength(schema, buffer, size);

  ASSERT_EQ(deserialized.size(), 3);
  EXPECT_TRUE(deserialized[0].IsNull());
  EXPECT_FALSE(deserialized[1].IsNull());
  EXPECT_DOUBLE_EQ(deserialized[1].GetDouble(), 3.14);
  EXPECT_TRUE(deserialized[2].IsNull());
}

TEST(TupleSerializerTest, SerializeVariableLengthVarChar) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Integer(1));
  values.push_back(FieldValue::VarChar("Alice"));

  char buffer[256];
  size_t size = TupleSerializer::SerializeVariableLength(schema, values, buffer,
                                                         sizeof(buffer));

  EXPECT_GT(size, 0);
}

TEST(TupleSerializerTest, RoundTripVariableLength) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.AddColumn("score", DataType::DOUBLE, false, 0);
  schema.Finalize();

  std::vector<FieldValue> original;
  original.push_back(FieldValue::Integer(42));
  original.push_back(FieldValue::VarChar("Hello, World!"));
  original.push_back(FieldValue::Double(99.9));

  char buffer[512];
  size_t size = TupleSerializer::SerializeVariableLength(
      schema, original, buffer, sizeof(buffer));

  auto deserialized =
      TupleSerializer::DeserializeVariableLength(schema, buffer, size);

  ASSERT_EQ(deserialized.size(), 3);
  EXPECT_EQ(deserialized[0].GetInteger(), 42);
  EXPECT_EQ(deserialized[1].GetString(), "Hello, World!");
  EXPECT_DOUBLE_EQ(deserialized[2].GetDouble(), 99.9);
}

TEST(TupleSerializerTest, VariableLengthEmptyString) {
  Schema schema;
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.Finalize();

  std::vector<FieldValue> original;
  original.push_back(FieldValue::VarChar(""));

  char buffer[256];
  size_t size = TupleSerializer::SerializeVariableLength(
      schema, original, buffer, sizeof(buffer));

  auto deserialized =
      TupleSerializer::DeserializeVariableLength(schema, buffer, size);

  ASSERT_EQ(deserialized.size(), 1);
  EXPECT_EQ(deserialized[0].GetString(), "");
}

TEST(TupleSerializerTest, VariableLengthWithNulls) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, true, 0);
  schema.AddColumn("name", DataType::VARCHAR, true, 100);
  schema.AddColumn("description", DataType::TEXT, true, 1000);
  schema.Finalize();

  std::vector<FieldValue> original;
  original.push_back(FieldValue::Integer(123));
  original.push_back(FieldValue::Null(DataType::VARCHAR));
  original.push_back(FieldValue::Text("Some text"));

  char buffer[512];
  size_t size = TupleSerializer::SerializeVariableLength(
      schema, original, buffer, sizeof(buffer));

  auto deserialized =
      TupleSerializer::DeserializeVariableLength(schema, buffer, size);

  ASSERT_EQ(deserialized.size(), 3);
  EXPECT_EQ(deserialized[0].GetInteger(), 123);
  EXPECT_TRUE(deserialized[1].IsNull());
  EXPECT_EQ(deserialized[2].GetString(), "Some text");
}

TEST(TupleSerializerTest, VariableLengthBlob) {
  Schema schema;
  schema.AddColumn("data", DataType::BLOB, false, 1000);
  schema.Finalize();

  std::vector<uint8_t> blob_data = {0x00, 0xFF, 0xAB, 0xCD, 0xEF};
  std::vector<FieldValue> original;
  original.push_back(FieldValue::Blob(blob_data));

  char buffer[256];
  size_t size = TupleSerializer::SerializeVariableLength(
      schema, original, buffer, sizeof(buffer));

  auto deserialized =
      TupleSerializer::DeserializeVariableLength(schema, buffer, size);

  ASSERT_EQ(deserialized.size(), 1);
  EXPECT_EQ(deserialized[0].GetBlob(), blob_data);
}

TEST(TupleSerializerTest, MixedFixedAndVariableTypes) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.AddColumn("age", DataType::TINYINT, false, 0);
  schema.AddColumn("email", DataType::TEXT, false, 200);
  schema.AddColumn("salary", DataType::DOUBLE, false, 0);
  schema.Finalize();

  std::vector<FieldValue> original;
  original.push_back(FieldValue::Integer(1001));
  original.push_back(FieldValue::VarChar("Alice"));
  original.push_back(FieldValue::TinyInt(30));
  original.push_back(FieldValue::Text("alice@example.com"));
  original.push_back(FieldValue::Double(75000.50));

  char buffer[1024];
  size_t size = TupleSerializer::SerializeVariableLength(
      schema, original, buffer, sizeof(buffer));

  auto deserialized =
      TupleSerializer::DeserializeVariableLength(schema, buffer, size);

  ASSERT_EQ(deserialized.size(), 5);
  EXPECT_EQ(deserialized[0].GetInteger(), 1001);
  EXPECT_EQ(deserialized[1].GetString(), "Alice");
  EXPECT_EQ(deserialized[2].GetTinyInt(), 30);
  EXPECT_EQ(deserialized[3].GetString(), "alice@example.com");
  EXPECT_DOUBLE_EQ(deserialized[4].GetDouble(), 75000.50);
}

TEST(TupleSerializerTest, LargeText) {
  Schema schema;
  schema.AddColumn("content", DataType::TEXT, false, 10000);
  schema.Finalize();

  std::string large_text(5000, 'x');
  std::vector<FieldValue> original;
  original.push_back(FieldValue::Text(large_text));

  char buffer[10000];
  size_t size = TupleSerializer::SerializeVariableLength(
      schema, original, buffer, sizeof(buffer));

  auto deserialized =
      TupleSerializer::DeserializeVariableLength(schema, buffer, size);

  ASSERT_EQ(deserialized.size(), 1);
  EXPECT_EQ(deserialized[0].GetString(), large_text);
}

TEST(TupleSerializerTest, CalculateSerializedSizeFixedLength) {
  Schema schema;
  schema.AddColumn("col_int", DataType::INTEGER, false, 0);
  schema.AddColumn("col_double", DataType::DOUBLE, false, 0);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Integer(123));
  values.push_back(FieldValue::Double(45.6));

  size_t calculated_size =
      TupleSerializer::CalculateSerializedSize(schema, values);

  char buffer[256];
  size_t actual_size = TupleSerializer::SerializeFixedLength(
      schema, values, buffer, sizeof(buffer));

  EXPECT_EQ(calculated_size, actual_size);
}

TEST(TupleSerializerTest, CalculateSerializedSizeVariableLength) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Integer(42));
  values.push_back(FieldValue::VarChar("Test"));

  size_t calculated_size =
      TupleSerializer::CalculateSerializedSize(schema, values);
  EXPECT_GT(calculated_size, 0);
}

TEST(TupleSerializerTest, BoundaryValuesBigInt) {
  Schema schema;
  schema.AddColumn("min_val", DataType::BIGINT, false, 0);
  schema.AddColumn("max_val", DataType::BIGINT, false, 0);
  schema.Finalize();

  std::vector<FieldValue> original;
  original.push_back(FieldValue::BigInt(std::numeric_limits<int64_t>::min()));
  original.push_back(FieldValue::BigInt(std::numeric_limits<int64_t>::max()));

  char buffer[256];
  size_t size = TupleSerializer::SerializeFixedLength(schema, original, buffer,
                                                      sizeof(buffer));

  auto deserialized =
      TupleSerializer::DeserializeFixedLength(schema, buffer, size);

  ASSERT_EQ(deserialized.size(), 2);
  EXPECT_EQ(deserialized[0].GetBigInt(), std::numeric_limits<int64_t>::min());
  EXPECT_EQ(deserialized[1].GetBigInt(), std::numeric_limits<int64_t>::max());
}

TEST(TupleSerializerTest, ErrorSchemaNotFinalized) {
  Schema schema;
  schema.AddColumn("col1", DataType::INTEGER, false, 0);

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Integer(42));

  char buffer[256];
  EXPECT_THROW(TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                     sizeof(buffer)),
               std::runtime_error);
}

TEST(TupleSerializerTest, ErrorValueCountMismatch) {
  Schema schema;
  schema.AddColumn("col1", DataType::INTEGER, false, 0);
  schema.AddColumn("col2", DataType::DOUBLE, false, 0);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Integer(42));

  char buffer[256];
  EXPECT_THROW(TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                     sizeof(buffer)),
               std::runtime_error);
}

TEST(TupleSerializerTest, ErrorWrongSerializationMethod) {
  Schema schema;
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::VarChar("test"));

  char buffer[256];
  EXPECT_THROW(TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                     sizeof(buffer)),
               std::runtime_error);
}
