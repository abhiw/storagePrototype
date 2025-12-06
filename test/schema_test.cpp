//
// Created by Abhi on 02/12/25.
//

#include "../include/schema/schema.h"

#include <gtest/gtest.h>

// Basic schema creation, finalize and offset/alignment expectations
TEST(SchemaTest, AddAndFinalizeOffsetsAndSizes) {
  Schema s;

  // Add fixed-length columns and one variable-length nullable column
  s.AddColumn("id", INTEGER, false, 4);
  s.AddColumn("value", DOUBLE, false, 8);
  s.AddColumn("text", CHAR, true, 0);  // variable-length (size 0)

  EXPECT_EQ(s.GetColumnCount(), 3);
  EXPECT_TRUE(s.HasColumn("id"));
  EXPECT_TRUE(s.HasColumn("text"));
  EXPECT_FALSE(s.HasColumn("missing"));

  s.Finalize();

  EXPECT_TRUE(s.IsFinalized());

  // One nullable column → 1 byte bitmap
  EXPECT_EQ(s.GetNullBitmapSize(), 1);

  // Not all fixed-length because "text" is variable
  EXPECT_FALSE(s.IsFixedLength());

  // Expected layout (bytes): null bitmap = 1, align id to 4 -> offset 4,
  // id(4) -> next 8, value aligned to 8 -> offset 8, value(8) -> next 16,
  // text is variable and placed at offset 16 → tuple size 16
  EXPECT_EQ(s.GetTupleSize(), 16);

  ColumnDefinition idcol = s.GetColumn("id");
  EXPECT_EQ(idcol.GetFieldIndex(), 0);
  EXPECT_EQ(idcol.GetOffset(), 4);
  EXPECT_EQ(idcol.GetFixedSize(), 4);

  ColumnDefinition valcol = s.GetColumn("value");
  EXPECT_EQ(valcol.GetFieldIndex(), 1);
  EXPECT_EQ(valcol.GetOffset(), 8);
  EXPECT_EQ(valcol.GetFixedSize(), 8);

  ColumnDefinition textcol = s.GetColumn("text");
  EXPECT_EQ(textcol.GetFieldIndex(), 2);
  EXPECT_EQ(textcol.GetOffset(), 16);
  EXPECT_EQ(textcol.GetFixedSize(), 0);
  EXPECT_TRUE(textcol.GetIsNullable());
}

TEST(SchemaTest, GetAlignmentDelegatesToAlignmentModule) {
  Schema s;
  EXPECT_EQ(s.GetAlignment(CHAR), 1);
  EXPECT_EQ(s.GetAlignment(SMALLINT), 2);
  EXPECT_EQ(s.GetAlignment(INTEGER), 4);
  EXPECT_EQ(s.GetAlignment(DOUBLE), 8);
}

TEST(SchemaTest, GetColumnNotFoundReturnsDefault) {
  Schema s;
  ColumnDefinition col = s.GetColumn("no_such_column");
  EXPECT_EQ(col.GetColumnName(), "");
  EXPECT_EQ(col.GetDataType(), BOOLEAN);
  EXPECT_FALSE(col.GetIsNullable());
}
