#include "../include/schema/schema.h"
#include "../include/storage/disk_manager.h"
#include "../include/storage/free_space_map.h"
#include "../include/storage/page_manager.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <vector>

#include "../include/common/logger.h"
#include "../include/tuple/tuple_accessor.h"
#include "../include/tuple/tuple_builder.h"
#include "../include/tuple/tuple_serializer.h"

uint64_t GetTimestamp() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

TEST(CRUDIntegrationTest, CompleteCRUDWorkflow) {
  // Setup
  uint64_t timestamp = GetTimestamp();
  std::string db_file = "/tmp/test/crud_test_" + std::to_string(timestamp) + ".db";
  std::string fsm_file = "/tmp/test/crud_test_" + std::to_string(timestamp) + ".fsm";

  DiskManager disk_manager(db_file);
  FreeSpaceMap fsm(fsm_file);
  PageManager page_manager(&disk_manager, &fsm);

  // Create schema
  Schema schema;
  schema.AddColumn("id", DataType::INTEGER, false, 0);
  schema.AddColumn("name", DataType::VARCHAR, false, 100);
  schema.AddColumn("salary", DataType::DOUBLE, false, 0);
  schema.AddColumn("department", DataType::VARCHAR, true, 50);
  schema.Finalize();

  TupleBuilder builder(schema);
  char buffer[1024];
  char read_buffer[1024];

  // TEST 1: INSERT single row
  auto values = builder.SetInteger("id", 1001)
                    .SetVarChar("name", "Alice Johnson")
                    .SetDouble("salary", 75000.50)
                    .SetVarChar("department", "Engineering")
                    .Build();

  size_t size = TupleSerializer::SerializeVariableLength(schema, values, buffer, sizeof(buffer));
  TupleId tid1 = page_manager.InsertTuple(buffer, size);
  ASSERT_NE(tid1.slot_id, INVALID_SLOT_ID) << "Insert should succeed";

  // TEST 2: READ the inserted row
  ErrorCode result = page_manager.GetTuple(tid1, read_buffer, sizeof(read_buffer));
  ASSERT_EQ(result.code, 0) << "Read should succeed";

  TupleAccessor accessor(schema, read_buffer, size);
  EXPECT_EQ(accessor.GetInteger("id"), 1001);
  EXPECT_EQ(accessor.GetString("name"), "Alice Johnson");
  EXPECT_DOUBLE_EQ(accessor.GetDouble("salary"), 75000.50);
  EXPECT_FALSE(accessor.IsNull("department"));
  EXPECT_EQ(accessor.GetString("department"), "Engineering");

  // TEST 3: UPDATE the row
  auto updated_values = builder.SetInteger("id", 1001)
                            .SetVarChar("name", "Alice Smith")
                            .SetDouble("salary", 85000.75)
                            .SetVarChar("department", "Engineering")
                            .Build();

  size_t updated_size = TupleSerializer::SerializeVariableLength(schema, updated_values, buffer, sizeof(buffer));
  result = page_manager.UpdateTuple(tid1, buffer, updated_size);
  EXPECT_EQ(result.code, 0) << "Update should succeed";

  // Verify the update
  result = page_manager.GetTuple(tid1, read_buffer, sizeof(read_buffer));
  ASSERT_EQ(result.code, 0);

  TupleAccessor updated_accessor(schema, read_buffer, updated_size);
  EXPECT_EQ(updated_accessor.GetInteger("id"), 1001);
  EXPECT_EQ(updated_accessor.GetString("name"), "Alice Smith");
  EXPECT_DOUBLE_EQ(updated_accessor.GetDouble("salary"), 85000.75);

  // TEST 4: INSERT multiple rows
  std::vector<TupleId> tids;

  auto v2 = builder.SetInteger("id", 1002)
                .SetVarChar("name", "Bob Smith")
                .SetDouble("salary", 65000.00)
                .SetVarChar("department", "Sales")
                .Build();
  size = TupleSerializer::SerializeVariableLength(schema, v2, buffer, sizeof(buffer));
  tids.push_back(page_manager.InsertTuple(buffer, size));

  auto v3 = builder.SetInteger("id", 1003)
                .SetVarChar("name", "Charlie Brown")
                .SetDouble("salary", 95000.75)
                .SetNull("department")  // NULL department
                .Build();
  size = TupleSerializer::SerializeVariableLength(schema, v3, buffer, sizeof(buffer));
  tids.push_back(page_manager.InsertTuple(buffer, size));

  // Verify all inserts succeeded
  for (const auto& tid : tids) {
    ASSERT_NE(tid.slot_id, INVALID_SLOT_ID);
  }

  // TEST 5: Read and verify NULL handling
  result = page_manager.GetTuple(tids[1], read_buffer, sizeof(read_buffer));
  ASSERT_EQ(result.code, 0);
  TupleAccessor null_accessor(schema, read_buffer, size);
  EXPECT_TRUE(null_accessor.IsNull("department")) << "Department should be NULL";

  // TEST 6: DELETE rows
  result = page_manager.DeleteTuple(tid1);
  EXPECT_EQ(result.code, 0) << "Delete should succeed";

  // Verify deleted tuple cannot be read
  result = page_manager.GetTuple(tid1, read_buffer, sizeof(read_buffer));
  EXPECT_NE(result.code, 0) << "Read deleted tuple should fail";

  // Verify non-deleted tuples can still be read
  result = page_manager.GetTuple(tids[0], read_buffer, sizeof(read_buffer));
  EXPECT_EQ(result.code, 0) << "Non-deleted tuple should be readable";

  // TEST 7: Edge cases - empty string
  auto empty_val = builder.SetInteger("id", 2001)
                       .SetVarChar("name", "")
                       .SetDouble("salary", 50000.00)
                       .SetVarChar("department", "")
                       .Build();
  size = TupleSerializer::SerializeVariableLength(schema, empty_val, buffer, sizeof(buffer));
  TupleId tid_empty = page_manager.InsertTuple(buffer, size);
  ASSERT_NE(tid_empty.slot_id, INVALID_SLOT_ID);

  result = page_manager.GetTuple(tid_empty, read_buffer, sizeof(read_buffer));
  ASSERT_EQ(result.code, 0);
  TupleAccessor empty_accessor(schema, read_buffer, size);
  EXPECT_EQ(empty_accessor.GetString("name"), "");

  // TEST 8: Large values
  std::string long_name(100, 'X');
  auto large_val = builder.SetInteger("id", 3001)
                       .SetVarChar("name", long_name)
                       .SetDouble("salary", 999999999.99)
                       .SetVarChar("department", "Executive")
                       .Build();
  size = TupleSerializer::SerializeVariableLength(schema, large_val, buffer, sizeof(buffer));
  TupleId tid_large = page_manager.InsertTuple(buffer, size);
  ASSERT_NE(tid_large.slot_id, INVALID_SLOT_ID);

  result = page_manager.GetTuple(tid_large, read_buffer, sizeof(read_buffer));
  ASSERT_EQ(result.code, 0);
  TupleAccessor large_accessor(schema, read_buffer, size);
  EXPECT_EQ(large_accessor.GetString("name").length(), 100);
  EXPECT_DOUBLE_EQ(large_accessor.GetDouble("salary"), 999999999.99);

  // Cleanup
  std::remove(db_file.c_str());
  std::remove(fsm_file.c_str());
}
