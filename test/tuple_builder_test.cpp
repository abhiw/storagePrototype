#include "../include/tuple/tuple_builder.h"

#include <gtest/gtest.h>

#include "../include/tuple/tuple_serializer.h"

TEST(TupleBuilderTest, BuildSimpleTuple) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("score", DataType::DOUBLE, false, 0);
  schema.Finalize();

  TupleBuilder builder(schema);
  auto values = builder.SetInteger("id", 42).SetDouble("score", 98.6).Build();

  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0].GetInteger(), 42);
  EXPECT_DOUBLE_EQ(values[1].GetDouble(), 98.6);
}

TEST(TupleBuilderTest, BuildWithAllTypes) {
  Schema schema;
  schema.AddColumn("col_bool", DataType::BOOLEAN, false, 0);
  schema.AddColumn("col_tinyint", DataType::TINYINT, false, 0);
  schema.AddColumn("col_smallint", DataType::SMALLINT, false, 0);
  schema.AddColumn("col_int", DataType::INTEGER, false, 0);
  schema.AddColumn("col_bigint", DataType::BIGINT, false, 0);
  schema.AddColumn("col_float", DataType::FLOAT, false, 0);
  schema.AddColumn("col_double", DataType::DOUBLE, false, 0);
  schema.Finalize();

  TupleBuilder builder(schema);
  auto values = builder.SetBoolean("col_bool", true)
                    .SetTinyInt("col_tinyint", 10)
                    .SetSmallInt("col_smallint", 1000)
                    .SetInteger("col_int", 100000)
                    .SetBigInt("col_bigint", 1000000000LL)
                    .SetFloat("col_float", 3.14f)
                    .SetDouble("col_double", 2.718)
                    .Build();

  EXPECT_EQ(values.size(), 7);
  EXPECT_TRUE(values[0].GetBoolean());
  EXPECT_EQ(values[1].GetTinyInt(), 10);
  EXPECT_EQ(values[2].GetSmallInt(), 1000);
  EXPECT_EQ(values[3].GetInteger(), 100000);
  EXPECT_EQ(values[4].GetBigInt(), 1000000000LL);
  EXPECT_FLOAT_EQ(values[5].GetFloat(), 3.14f);
  EXPECT_DOUBLE_EQ(values[6].GetDouble(), 2.718);
}

TEST(TupleBuilderTest, BuildWithVariableLengthTypes) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.AddColumn("description", DataType::TEXT, false, 1000);
  schema.Finalize();

  TupleBuilder builder(schema);
  auto values = builder.SetInteger("id", 1)
                    .SetVarChar("name", "Alice")
                    .SetText("description", "A test description")
                    .Build();

  EXPECT_EQ(values.size(), 3);
  EXPECT_EQ(values[0].GetInteger(), 1);
  EXPECT_EQ(values[1].GetString(), "Alice");
  EXPECT_EQ(values[2].GetString(), "A test description");
}

TEST(TupleBuilderTest, BuildWithNull) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("optional", DataType::VARCHAR, true, 100);
  schema.Finalize();

  TupleBuilder builder(schema);
  auto values = builder.SetInteger("id", 42).SetNull("optional").Build();

  EXPECT_EQ(values.size(), 2);
  EXPECT_EQ(values[0].GetInteger(), 42);
  EXPECT_TRUE(values[1].IsNull());
}

TEST(TupleBuilderTest, IndexBasedAccess) {
  Schema schema;
  schema.AddColumn("col1", DataType::INTEGER, false, 0);
  schema.AddColumn("col2", DataType::DOUBLE, false, 0);
  schema.Finalize();

  TupleBuilder builder(schema);
  auto values = builder.SetInteger(0, 100).SetDouble(1, 50.5).Build();

  EXPECT_EQ(values.size(), 2);
  EXPECT_EQ(values[0].GetInteger(), 100);
  EXPECT_DOUBLE_EQ(values[1].GetDouble(), 50.5);
}

TEST(TupleBuilderTest, Reset) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.Finalize();

  TupleBuilder builder(schema);
  builder.SetInteger("id", 42);
  builder.Reset();
  builder.SetInteger("id", 100);
  auto values = builder.Build();

  EXPECT_EQ(values.size(), 1);
  EXPECT_EQ(values[0].GetInteger(), 100);
}

TEST(TupleBuilderTest, ErrorTypeMismatch) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.Finalize();

  TupleBuilder builder(schema);
  EXPECT_THROW(builder.SetDouble("id", 3.14), std::runtime_error);
}

TEST(TupleBuilderTest, ErrorColumnNotFound) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.Finalize();

  TupleBuilder builder(schema);
  EXPECT_THROW(builder.SetInteger("nonexistent", 42), std::runtime_error);
}

TEST(TupleBuilderTest, ErrorFieldIndexOutOfBounds) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.Finalize();

  TupleBuilder builder(schema);
  EXPECT_THROW(builder.SetInteger(10, 42), std::runtime_error);
}

TEST(TupleBuilderTest, ErrorMissingNonNullableField) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("score", DataType::DOUBLE, false, 0);
  schema.Finalize();

  TupleBuilder builder(schema);
  builder.SetInteger("id", 42);
  EXPECT_THROW(builder.Build(), std::runtime_error);
}

TEST(TupleBuilderTest, ErrorSetNullOnNonNullableField) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.Finalize();

  TupleBuilder builder(schema);
  EXPECT_THROW(builder.SetNull("id"), std::runtime_error);
}

TEST(TupleBuilderTest, IntegrationWithSerializer) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.Finalize();

  TupleBuilder builder(schema);
  auto values =
      builder.SetInteger("id", 123).SetVarChar("name", "TestUser").Build();

  char buffer[256];
  size_t size = TupleSerializer::SerializeVariableLength(schema, values, buffer,
                                                         sizeof(buffer));
  EXPECT_GT(size, 0);

  auto deserialized =
      TupleSerializer::DeserializeVariableLength(schema, buffer, size);
  EXPECT_EQ(deserialized[0].GetInteger(), 123);
  EXPECT_EQ(deserialized[1].GetString(), "TestUser");
}
