#include "../include/tuple/tuple_accessor.h"

#include <gtest/gtest.h>

#include "../include/tuple/tuple_builder.h"
#include "../include/tuple/tuple_serializer.h"

TEST(TupleAccessorTest, ReadFixedLengthTuple) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("score", DataType::DOUBLE, false, 0);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Integer(42));
  values.push_back(FieldValue::Double(98.6));

  char buffer[256];
  size_t size = TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                      sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_EQ(accessor.GetInteger("id"), 42);
  EXPECT_DOUBLE_EQ(accessor.GetDouble("score"), 98.6);
}

TEST(TupleAccessorTest, ReadVariableLengthTuple) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Integer(123));
  values.push_back(FieldValue::VarChar("Alice"));

  char buffer[256];
  size_t size = TupleSerializer::SerializeVariableLength(schema, values, buffer,
                                                         sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_EQ(accessor.GetInteger("id"), 123);
  EXPECT_EQ(accessor.GetString("name"), "Alice");
}

TEST(TupleAccessorTest, IndexBasedAccess) {
  Schema schema;
  schema.AddColumn("col1", DataType::INTEGER, false, 0);
  schema.AddColumn("col2", DataType::DOUBLE, false, 0);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Integer(100));
  values.push_back(FieldValue::Double(50.5));

  char buffer[256];
  size_t size = TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                      sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_EQ(accessor.GetInteger(0), 100);
  EXPECT_DOUBLE_EQ(accessor.GetDouble(1), 50.5);
}

TEST(TupleAccessorTest, ReadNullValues) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, true, 0);
  schema.AddColumn("name", DataType::VARCHAR, true, 100);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Null(DataType::INTEGER));
  values.push_back(FieldValue::VarChar("Alice"));

  char buffer[256];
  size_t size = TupleSerializer::SerializeVariableLength(schema, values, buffer,
                                                         sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_TRUE(accessor.IsNull("id"));
  EXPECT_FALSE(accessor.IsNull("name"));
  EXPECT_EQ(accessor.GetString("name"), "Alice");
  EXPECT_THROW(accessor.GetInteger("id"), std::runtime_error);
}

TEST(TupleAccessorTest, ReadAllTypes) {
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
  values.push_back(FieldValue::TinyInt(10));
  values.push_back(FieldValue::SmallInt(1000));
  values.push_back(FieldValue::Integer(100000));
  values.push_back(FieldValue::BigInt(1000000000LL));
  values.push_back(FieldValue::Float(3.14f));
  values.push_back(FieldValue::Double(2.718));

  char buffer[256];
  size_t size = TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                      sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_TRUE(accessor.GetBoolean("col_bool"));
  EXPECT_EQ(accessor.GetTinyInt("col_tinyint"), 10);
  EXPECT_EQ(accessor.GetSmallInt("col_smallint"), 1000);
  EXPECT_EQ(accessor.GetInteger("col_int"), 100000);
  EXPECT_EQ(accessor.GetBigInt("col_bigint"), 1000000000LL);
  EXPECT_FLOAT_EQ(accessor.GetFloat("col_float"), 3.14f);
  EXPECT_DOUBLE_EQ(accessor.GetDouble("col_double"), 2.718);
}

TEST(TupleAccessorTest, GetFieldValue) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Integer(42));

  char buffer[256];
  size_t size = TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                      sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  FieldValue fv = accessor.GetFieldValue("id");
  EXPECT_EQ(fv.GetInteger(), 42);
}

TEST(TupleAccessorTest, ErrorTypeMismatch) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Integer(42));

  char buffer[256];
  size_t size = TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                      sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_THROW(accessor.GetDouble("id"), std::runtime_error);
}

TEST(TupleAccessorTest, ErrorColumnNotFound) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Integer(42));

  char buffer[256];
  size_t size = TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                      sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_THROW(accessor.GetInteger("nonexistent"), std::runtime_error);
}

TEST(TupleAccessorTest, ErrorFieldIndexOutOfBounds) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Integer(42));

  char buffer[256];
  size_t size = TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                      sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_THROW(accessor.GetInteger(10), std::runtime_error);
}

TEST(TupleAccessorTest, IntegrationWithBuilder) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.AddColumn("score", DataType::DOUBLE, false, 0);
  schema.Finalize();

  TupleBuilder builder(schema);
  auto values = builder.SetInteger("id", 1001)
                    .SetVarChar("name", "TestUser")
                    .SetDouble("score", 95.5)
                    .Build();

  char buffer[512];
  size_t size = TupleSerializer::SerializeVariableLength(schema, values, buffer,
                                                         sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_EQ(accessor.GetInteger("id"), 1001);
  EXPECT_EQ(accessor.GetString("name"), "TestUser");
  EXPECT_DOUBLE_EQ(accessor.GetDouble("score"), 95.5);
}

TEST(TupleAccessorTest, LazyDeserialization) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.Finalize();

  std::vector<FieldValue> values;
  values.push_back(FieldValue::Integer(42));
  values.push_back(FieldValue::VarChar("Alice"));

  char buffer[256];
  size_t size = TupleSerializer::SerializeVariableLength(schema, values, buffer,
                                                         sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);

  EXPECT_FALSE(accessor.IsNull("id"));
  EXPECT_EQ(accessor.GetInteger("id"), 42);
  EXPECT_EQ(accessor.GetString("name"), "Alice");
}
