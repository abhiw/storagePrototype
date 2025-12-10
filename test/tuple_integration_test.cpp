#include <gtest/gtest.h>

#include "../include/tuple/tuple_accessor.h"
#include "../include/tuple/tuple_builder.h"
#include "../include/tuple/tuple_serializer.h"

TEST(TupleIntegrationTest, EndToEndFixedLength) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("age", DataType::TINYINT, false, 0);
  schema.AddColumn("salary", DataType::DOUBLE, false, 0);
  schema.Finalize();

  TupleBuilder builder(schema);
  auto values = builder.SetInteger("id", 1)
                    .SetTinyInt("age", 25)
                    .SetDouble("salary", 75000.50)
                    .Build();

  char buffer[512];
  size_t size = TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                      sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_EQ(accessor.GetInteger("id"), 1);
  EXPECT_EQ(accessor.GetTinyInt("age"), 25);
  EXPECT_DOUBLE_EQ(accessor.GetDouble("salary"), 75000.50);
}

TEST(TupleIntegrationTest, EndToEndVariableLength) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.AddColumn("email", DataType::TEXT, false, 200);
  schema.AddColumn("score", DataType::DOUBLE, false, 0);
  schema.Finalize();

  TupleBuilder builder(schema);
  auto values = builder.SetInteger("id", 1001)
                    .SetVarChar("name", "Alice Johnson")
                    .SetText("email", "alice.johnson@example.com")
                    .SetDouble("score", 95.5)
                    .Build();

  char buffer[1024];
  size_t size = TupleSerializer::SerializeVariableLength(schema, values, buffer,
                                                         sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_EQ(accessor.GetInteger("id"), 1001);
  EXPECT_EQ(accessor.GetString("name"), "Alice Johnson");
  EXPECT_EQ(accessor.GetString("email"), "alice.johnson@example.com");
  EXPECT_DOUBLE_EQ(accessor.GetDouble("score"), 95.5);
}

TEST(TupleIntegrationTest, ComplexSchemaAllTypes) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("active", DataType::BOOLEAN, false, 0);
  schema.AddColumn("level", DataType::TINYINT, false, 0);
  schema.AddColumn("points", DataType::SMALLINT, false, 0);
  schema.AddColumn("score", DataType::BIGINT, false, 0);
  schema.AddColumn("ratio", DataType::FLOAT, false, 0);
  schema.AddColumn("average", DataType::DOUBLE, false, 0);
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.AddColumn("description", DataType::TEXT, false, 1000);
  schema.Finalize();

  TupleBuilder builder(schema);
  auto values = builder.SetInteger("id", 1)
                    .SetBoolean("active", true)
                    .SetTinyInt("level", 5)
                    .SetSmallInt("points", 1500)
                    .SetBigInt("score", 9876543210LL)
                    .SetFloat("ratio", 0.95f)
                    .SetDouble("average", 87.65)
                    .SetVarChar("name", "Test User")
                    .SetText("description", "This is a comprehensive test")
                    .Build();

  char buffer[2048];
  size_t size = TupleSerializer::SerializeVariableLength(schema, values, buffer,
                                                         sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_EQ(accessor.GetInteger("id"), 1);
  EXPECT_TRUE(accessor.GetBoolean("active"));
  EXPECT_EQ(accessor.GetTinyInt("level"), 5);
  EXPECT_EQ(accessor.GetSmallInt("points"), 1500);
  EXPECT_EQ(accessor.GetBigInt("score"), 9876543210LL);
  EXPECT_FLOAT_EQ(accessor.GetFloat("ratio"), 0.95f);
  EXPECT_DOUBLE_EQ(accessor.GetDouble("average"), 87.65);
  EXPECT_EQ(accessor.GetString("name"), "Test User");
  EXPECT_EQ(accessor.GetString("description"), "This is a comprehensive test");
}

TEST(TupleIntegrationTest, MixedNullValues) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("optional1", DataType::VARCHAR, true, 100);
  schema.AddColumn("required", DataType::DOUBLE, false, 0);
  schema.AddColumn("optional2", DataType::TEXT, true, 200);
  schema.Finalize();

  TupleBuilder builder(schema);
  auto values = builder.SetInteger("id", 42)
                    .SetNull("optional1")
                    .SetDouble("required", 3.14)
                    .SetText("optional2", "Present")
                    .Build();

  char buffer[512];
  size_t size = TupleSerializer::SerializeVariableLength(schema, values, buffer,
                                                         sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_EQ(accessor.GetInteger("id"), 42);
  EXPECT_TRUE(accessor.IsNull("optional1"));
  EXPECT_DOUBLE_EQ(accessor.GetDouble("required"), 3.14);
  EXPECT_FALSE(accessor.IsNull("optional2"));
  EXPECT_EQ(accessor.GetString("optional2"), "Present");
}

TEST(TupleIntegrationTest, EmptyStrings) {
  Schema schema;
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.AddColumn("description", DataType::TEXT, false, 1000);
  schema.Finalize();

  TupleBuilder builder(schema);
  auto values =
      builder.SetVarChar("name", "").SetText("description", "").Build();

  char buffer[512];
  size_t size = TupleSerializer::SerializeVariableLength(schema, values, buffer,
                                                         sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_EQ(accessor.GetString("name"), "");
  EXPECT_EQ(accessor.GetString("description"), "");
}

TEST(TupleIntegrationTest, LargeVariableLengthData) {
  Schema schema;
  schema.AddColumn("content", DataType::TEXT, false, 10000);
  schema.Finalize();

  std::string large_text(5000, 'X');
  TupleBuilder builder(schema);
  auto values = builder.SetText("content", large_text).Build();

  char buffer[10000];
  size_t size = TupleSerializer::SerializeVariableLength(schema, values, buffer,
                                                         sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_EQ(accessor.GetString("content"), large_text);
}

TEST(TupleIntegrationTest, BlobData) {
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("data", DataType::BLOB, false, 1000);
  schema.Finalize();

  std::vector<uint8_t> blob_data(256);
  for (size_t i = 0; i < blob_data.size(); i++) {
    blob_data[i] = static_cast<uint8_t>(i);
  }

  TupleBuilder builder(schema);
  auto values =
      builder.SetInteger("id", 999).SetBlob("data", blob_data).Build();

  char buffer[1024];
  size_t size = TupleSerializer::SerializeVariableLength(schema, values, buffer,
                                                         sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_EQ(accessor.GetInteger("id"), 999);
  EXPECT_EQ(accessor.GetBlob("data"), blob_data);
}

TEST(TupleIntegrationTest, MultipleRoundTrips) {
  Schema schema;
  schema.AddColumn("counter", DataType::INTEGER, false, 0);
  schema.Finalize();

  char buffer[256];

  for (int i = 0; i < 100; i++) {
    TupleBuilder builder(schema);
    auto values = builder.SetInteger("counter", i).Build();

    size_t size = TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                        sizeof(buffer));

    TupleAccessor accessor(schema, buffer, size);
    EXPECT_EQ(accessor.GetInteger("counter"), i);
  }
}

TEST(TupleIntegrationTest, LargeSchema64Fields) {
  Schema schema;
  for (int i = 0; i < 32; i++) {
    schema.AddColumn("int_col_" + std::to_string(i), DataType::INTEGER, false,
                     0);
  }
  for (int i = 0; i < 32; i++) {
    schema.AddColumn("double_col_" + std::to_string(i), DataType::DOUBLE, false,
                     0);
  }
  schema.Finalize();

  TupleBuilder builder(schema);
  for (int i = 0; i < 32; i++) {
    builder.SetInteger("int_col_" + std::to_string(i), i * 10);
  }
  for (int i = 0; i < 32; i++) {
    builder.SetDouble("double_col_" + std::to_string(i), i * 1.5);
  }
  auto values = builder.Build();

  char buffer[2048];
  size_t size = TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                      sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  for (int i = 0; i < 32; i++) {
    EXPECT_EQ(accessor.GetInteger("int_col_" + std::to_string(i)), i * 10);
  }
  for (int i = 0; i < 32; i++) {
    EXPECT_DOUBLE_EQ(accessor.GetDouble("double_col_" + std::to_string(i)),
                     i * 1.5);
  }
}

TEST(TupleIntegrationTest, AlignmentVerification) {
  Schema schema;
  schema.AddColumn("tinyint_col", DataType::TINYINT, false, 0);
  schema.AddColumn("int_col", DataType::INTEGER, false, 0);
  schema.AddColumn("double_col", DataType::DOUBLE, false, 0);
  schema.Finalize();

  TupleBuilder builder(schema);
  auto values = builder.SetTinyInt("tinyint_col", 1)
                    .SetInteger("int_col", 1000)
                    .SetDouble("double_col", 123.456)
                    .Build();

  char buffer[256];
  size_t size = TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                      sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_EQ(accessor.GetTinyInt("tinyint_col"), 1);
  EXPECT_EQ(accessor.GetInteger("int_col"), 1000);
  EXPECT_DOUBLE_EQ(accessor.GetDouble("double_col"), 123.456);
}

TEST(TupleIntegrationTest, BoundaryValues) {
  Schema schema;
  schema.AddColumn("min_int", DataType::INTEGER, false, 0);
  schema.AddColumn("max_int", DataType::INTEGER, false, 0);
  schema.AddColumn("min_bigint", DataType::BIGINT, false, 0);
  schema.AddColumn("max_bigint", DataType::BIGINT, false, 0);
  schema.Finalize();

  TupleBuilder builder(schema);
  auto values =
      builder.SetInteger("min_int", std::numeric_limits<int32_t>::min())
          .SetInteger("max_int", std::numeric_limits<int32_t>::max())
          .SetBigInt("min_bigint", std::numeric_limits<int64_t>::min())
          .SetBigInt("max_bigint", std::numeric_limits<int64_t>::max())
          .Build();

  char buffer[512];
  size_t size = TupleSerializer::SerializeFixedLength(schema, values, buffer,
                                                      sizeof(buffer));

  TupleAccessor accessor(schema, buffer, size);
  EXPECT_EQ(accessor.GetInteger("min_int"),
            std::numeric_limits<int32_t>::min());
  EXPECT_EQ(accessor.GetInteger("max_int"),
            std::numeric_limits<int32_t>::max());
  EXPECT_EQ(accessor.GetBigInt("min_bigint"),
            std::numeric_limits<int64_t>::min());
  EXPECT_EQ(accessor.GetBigInt("max_bigint"),
            std::numeric_limits<int64_t>::max());
}
