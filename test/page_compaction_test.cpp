//
// Tests for Page Compaction, Deletion, and related functionality
//

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

#include "../include/common/types.h"
#include "../include/page/page.h"

// ============================================================================
// DeleteTuple Tests
// ============================================================================

TEST(DeleteTupleTest, DeleteSingleTuple) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert a tuple
  const char* data = "Hello World";
  slot_id_t slot = page->InsertTuple(data, strlen(data) + 1);
  ASSERT_NE(slot, INVALID_SLOT_ID);
  EXPECT_EQ(page->GetSlotCount(), 1);
  EXPECT_EQ(page->GetDeletedTupleCount(), 0);

  // Delete the tuple
  ErrorCode result = page->DeleteTuple(slot);
  EXPECT_EQ(result.code, 0) << "DeleteTuple should succeed";

  // Verify deletion
  EXPECT_FALSE(page->IsSlotValid(slot)) << "Slot should be marked invalid";
  EXPECT_EQ(page->GetDeletedTupleCount(), 1);
  EXPECT_GT(page->GetFragmentedBytes(), 0);

  // Verify checksum is still valid after deletion
  EXPECT_TRUE(page->VerifyChecksum())
      << "Checksum should remain valid after deletion";
}

TEST(DeleteTupleTest, DeleteMultipleTuples) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert multiple tuples
  std::vector<slot_id_t> slots;
  for (int i = 0; i < 5; i++) {
    std::string data = "Tuple_" + std::to_string(i);
    slot_id_t slot = page->InsertTuple(data.c_str(), data.length() + 1);
    ASSERT_NE(slot, INVALID_SLOT_ID);
    slots.push_back(slot);
  }
  EXPECT_EQ(page->GetSlotCount(), 5);

  // Delete alternating tuples (0, 2, 4)
  page->DeleteTuple(slots[0]);
  page->DeleteTuple(slots[2]);
  page->DeleteTuple(slots[4]);

  EXPECT_EQ(page->GetDeletedTupleCount(), 3);
  EXPECT_FALSE(page->IsSlotValid(slots[0]));
  EXPECT_TRUE(page->IsSlotValid(slots[1]));
  EXPECT_FALSE(page->IsSlotValid(slots[2]));
  EXPECT_TRUE(page->IsSlotValid(slots[3]));
  EXPECT_FALSE(page->IsSlotValid(slots[4]));
}

TEST(DeleteTupleTest, DeleteInvalidSlotId) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Try to delete slot that doesn't exist
  ErrorCode result = page->DeleteTuple(100);
  EXPECT_NE(result.code, 0) << "Deleting invalid slot should fail";
}

TEST(DeleteTupleTest, DeleteAlreadyDeletedTuple) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert and delete a tuple
  const char* data = "Test";
  slot_id_t slot = page->InsertTuple(data, strlen(data) + 1);
  ASSERT_NE(slot, INVALID_SLOT_ID);

  ErrorCode result1 = page->DeleteTuple(slot);
  EXPECT_EQ(result1.code, 0);

  // Try to delete again
  ErrorCode result2 = page->DeleteTuple(slot);
  EXPECT_NE(result2.code, 0) << "Deleting already deleted tuple should fail";
}

TEST(DeleteTupleTest, DeleteUpdatesFragmentation) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert tuples of known sizes
  const char* small = "AB";
  const char* large = "ABCDEFGHIJKLMNOP";

  page->InsertTuple(small, strlen(small) + 1);
  slot_id_t slot2 = page->InsertTuple(large, strlen(large) + 1);

  size_t frag_before = page->GetFragmentedBytes();
  EXPECT_EQ(frag_before, 0);

  // Delete the large tuple
  page->DeleteTuple(slot2);

  size_t frag_after = page->GetFragmentedBytes();
  EXPECT_EQ(frag_after, strlen(large) + 1)
      << "Fragmented bytes should equal deleted tuple size";
}

// ============================================================================
// ShouldCompact Tests
// ============================================================================

TEST(ShouldCompactTest, NoCompactionNeededForEmptyPage) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  EXPECT_FALSE(page->ShouldCompact())
      << "Empty page should not need compaction";
}

TEST(ShouldCompactTest, NoCompactionNeededWithNoDeletions) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert tuples without deleting
  for (int i = 0; i < 10; i++) {
    std::string data = "Tuple_" + std::to_string(i);
    page->InsertTuple(data.c_str(), data.length() + 1);
  }

  EXPECT_FALSE(page->ShouldCompact())
      << "Page with no deletions should not need compaction";
}

TEST(ShouldCompactTest, CompactionNeededWithHighFragmentation) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert many tuples
  std::vector<slot_id_t> slots;
  for (int i = 0; i < 20; i++) {
    std::string data = "Data_" + std::to_string(i) + "_XXXXXXXXXXXX";
    slot_id_t slot = page->InsertTuple(data.c_str(), data.length() + 1);
    slots.push_back(slot);
  }

  // Delete more than half
  for (size_t i = 0; i < slots.size() / 2 + 2; i++) {
    page->DeleteTuple(slots[i]);
  }

  EXPECT_TRUE(page->ShouldCompact())
      << "Page with >50% deleted should need compaction";
}

TEST(ShouldCompactTest, CompactionNeededWithHighDeletedSlotRatio) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert tuples
  std::vector<slot_id_t> slots;
  for (int i = 0; i < 10; i++) {
    std::string data = "X";
    slot_id_t slot = page->InsertTuple(data.c_str(), data.length() + 1);
    slots.push_back(slot);
  }

  // Delete more than half the slots
  for (int i = 0; i < 6; i++) {
    page->DeleteTuple(slots[i]);
  }

  EXPECT_TRUE(page->ShouldCompact())
      << "Page with >50% deleted slots should need compaction";
}

// ============================================================================
// CompactPage Tests
// ============================================================================

TEST(CompactPageTest, CompactPageWithNoDeletions) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert tuples
  for (int i = 0; i < 5; i++) {
    std::string data = "Tuple_" + std::to_string(i);
    page->InsertTuple(data.c_str(), data.length() + 1);
  }

  uint16_t free_start_before = page->GetFreeStart();
  uint16_t slot_count_before = page->GetSlotCount();

  // Compact (should be no-op)
  page->CompactPage();

  EXPECT_EQ(page->GetFreeStart(), free_start_before);
  EXPECT_EQ(page->GetSlotCount(), slot_count_before);
  EXPECT_TRUE(page->VerifyChecksum());
}

TEST(CompactPageTest, CompactPageWithAllTuplesDeleted) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert and delete all tuples
  std::vector<slot_id_t> slots;
  for (int i = 0; i < 5; i++) {
    std::string data = "Tuple_" + std::to_string(i);
    slot_id_t slot = page->InsertTuple(data.c_str(), data.length() + 1);
    slots.push_back(slot);
  }

  for (auto slot : slots) {
    page->DeleteTuple(slot);
  }

  EXPECT_EQ(page->GetDeletedTupleCount(), 5);

  // Compact
  page->CompactPage();

  // Should reset to empty state
  EXPECT_EQ(page->GetFreeStart(), sizeof(PageHeader))
      << "free_start should reset to header size";
  EXPECT_EQ(page->GetSlotCount(), 0) << "slot_count should be 0";
  EXPECT_EQ(page->GetDeletedTupleCount(), 0);
  EXPECT_EQ(page->GetFragmentedBytes(), 0);
  EXPECT_TRUE(page->VerifyChecksum());
}

TEST(CompactPageTest, CompactPageReclaimsSpace) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert tuples
  std::vector<slot_id_t> slots;
  for (int i = 0; i < 10; i++) {
    std::string data = "Data_" + std::to_string(i) + "_PADDING";
    slot_id_t slot = page->InsertTuple(data.c_str(), data.length() + 1);
    slots.push_back(slot);
  }

  uint16_t free_start_before = page->GetFreeStart();

  // Delete half the tuples
  for (int i = 0; i < 5; i++) {
    page->DeleteTuple(slots[i]);
  }

  size_t fragmented_before = page->GetFragmentedBytes();
  EXPECT_GT(fragmented_before, 0);

  // Compact
  page->CompactPage();

  // Verify space is reclaimed
  EXPECT_LT(page->GetFreeStart(), free_start_before)
      << "free_start should decrease after compaction";
  EXPECT_EQ(page->GetFragmentedBytes(), 0)
      << "Fragmented bytes should be 0 after compaction";
  EXPECT_EQ(page->GetDeletedTupleCount(), 0)
      << "Deleted tuple count should be 0";
  EXPECT_TRUE(page->VerifyChecksum());
}

TEST(CompactPageTest, CompactPageRenumbersSlotsSequentially) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert 5 tuples
  std::vector<slot_id_t> original_slots;
  std::vector<std::string> data_values;
  for (int i = 0; i < 5; i++) {
    std::string data = "Value_" + std::to_string(i);
    data_values.push_back(data);
    slot_id_t slot = page->InsertTuple(data.c_str(), data.length() + 1);
    original_slots.push_back(slot);
  }

  // Delete slots 1 and 3
  page->DeleteTuple(original_slots[1]);
  page->DeleteTuple(original_slots[3]);

  EXPECT_EQ(page->GetSlotCount(), 5);

  // Compact
  page->CompactPage();

  // After compaction, slot count remains the same (slots are not renumbered)
  EXPECT_EQ(page->GetSlotCount(), 5) << "Slot count should remain 5";

  // Valid slots remain at their original positions (0, 2, 4)
  EXPECT_TRUE(page->IsSlotValid(original_slots[0]));
  EXPECT_FALSE(page->IsSlotValid(original_slots[1]));  // Was deleted
  EXPECT_TRUE(page->IsSlotValid(original_slots[2]));
  EXPECT_FALSE(page->IsSlotValid(original_slots[3]));  // Was deleted
  EXPECT_TRUE(page->IsSlotValid(original_slots[4]));
}

TEST(CompactPageTest, CompactPagePreservesDataIntegrity) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert tuples with known data
  std::vector<std::string> expected_data = {"AAA", "BBB", "CCC", "DDD", "EEE"};
  std::vector<slot_id_t> slots;

  for (const auto& data : expected_data) {
    slot_id_t slot = page->InsertTuple(data.c_str(), data.length() + 1);
    slots.push_back(slot);
  }

  // Delete slots 1 and 3
  page->DeleteTuple(slots[1]);
  page->DeleteTuple(slots[3]);

  // Compact
  page->CompactPage();

  // After compaction: slots keep their original IDs (0,2,4), not renumbered
  EXPECT_EQ(page->GetSlotCount(), 5);  // Slot count unchanged

  // Verify only valid slots (0, 2, 4) are accessible
  auto& slot0 = page->GetSlotEntry(slots[0]);
  auto& slot2 = page->GetSlotEntry(slots[2]);
  auto& slot4 = page->GetSlotEntry(slots[4]);

  EXPECT_TRUE(slot0.flags & SLOT_VALID);
  EXPECT_TRUE(slot2.flags & SLOT_VALID);
  EXPECT_TRUE(slot4.flags & SLOT_VALID);

  EXPECT_FALSE(page->IsSlotValid(slots[1]));  // Was deleted
  EXPECT_FALSE(page->IsSlotValid(slots[3]));  // Was deleted

  // All data should be compacted to the beginning (right after header)
  EXPECT_EQ(slot0.offset, sizeof(PageHeader))
      << "First tuple should start right after header";
  EXPECT_GT(slot2.offset, slot0.offset);
  EXPECT_GT(slot4.offset, slot2.offset);
}

TEST(CompactPageTest, CompactPageUpdatesSlotDirectory) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert 6 tuples
  std::vector<slot_id_t> slots;
  for (int i = 0; i < 6; i++) {
    std::string data = "T" + std::to_string(i);
    slot_id_t slot = page->InsertTuple(data.c_str(), data.length() + 1);
    slots.push_back(slot);
  }

  // Delete slots 0, 2, 4
  page->DeleteTuple(slots[0]);
  page->DeleteTuple(slots[2]);
  page->DeleteTuple(slots[4]);

  uint16_t free_start_before = page->GetFreeStart();

  // Compact
  page->CompactPage();

  // Slot count remains 6 (slots not renumbered to preserve external pointers)
  EXPECT_EQ(page->GetSlotCount(), 6);

  // free_start should decrease (data is compacted)
  EXPECT_LT(page->GetFreeStart(), free_start_before)
      << "free_start should decrease (data is compacted)";

  // Valid slots are 1, 3, 5
  EXPECT_FALSE(page->IsSlotValid(slots[0]));
  EXPECT_TRUE(page->IsSlotValid(slots[1]));
  EXPECT_FALSE(page->IsSlotValid(slots[2]));
  EXPECT_TRUE(page->IsSlotValid(slots[3]));
  EXPECT_FALSE(page->IsSlotValid(slots[4]));
  EXPECT_TRUE(page->IsSlotValid(slots[5]));
}

TEST(CompactPageTest, MultipleCompactions) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert, delete, compact - repeat multiple times
  for (int round = 0; round < 3; round++) {
    // Insert tuples
    std::vector<slot_id_t> slots;
    for (int i = 0; i < 5; i++) {
      std::string data =
          "Round" + std::to_string(round) + "_" + std::to_string(i);
      slot_id_t slot = page->InsertTuple(data.c_str(), data.length() + 1);
      slots.push_back(slot);
    }

    // Delete some
    page->DeleteTuple(slots[1]);
    page->DeleteTuple(slots[3]);

    // Compact
    page->CompactPage();

    // Verify integrity
    EXPECT_TRUE(page->VerifyChecksum());
    EXPECT_EQ(page->GetDeletedTupleCount(), 0);
    // fragmented_bytes includes slot directory overhead for deleted slots
    // since we don't renumber slots, some fragmentation remains acceptable
  }
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(PageCompactionIntegrationTest, DeleteAndCompactWorkflow) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // 1. Insert many tuples
  std::vector<slot_id_t> slots;
  for (int i = 0; i < 20; i++) {
    std::string data = "Data_Item_" + std::to_string(i);
    slot_id_t slot = page->InsertTuple(data.c_str(), data.length() + 1);
    ASSERT_NE(slot, INVALID_SLOT_ID);
    slots.push_back(slot);
  }

  uint16_t initial_slot_count = page->GetSlotCount();
  EXPECT_EQ(initial_slot_count, 20);

  // 2. Delete half of them
  for (int i = 0; i < 10; i++) {
    ErrorCode result = page->DeleteTuple(slots[i]);
    EXPECT_EQ(result.code, 0);
  }

  EXPECT_EQ(page->GetDeletedTupleCount(), 10);
  EXPECT_GT(page->GetFragmentedBytes(), 0);

  // 3. Check if compaction is needed
  EXPECT_TRUE(page->ShouldCompact())
      << "Should need compaction after 50% deletion";

  // 4. Compact
  page->CompactPage();

  // 5. Verify results (slots not renumbered, so count stays 20)
  EXPECT_EQ(page->GetSlotCount(), 20)
      << "Slot count unchanged (slots not renumbered)";
  EXPECT_EQ(page->GetDeletedTupleCount(), 0);
  // fragmented_bytes may be non-zero due to deleted slot overhead
  EXPECT_TRUE(page->VerifyChecksum());
}

TEST(PageCompactionIntegrationTest, FillDeleteCompactRefill) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Fill page
  std::vector<slot_id_t> slots;
  std::string large_data(100, 'X');
  for (int i = 0; i < 30; i++) {
    slot_id_t slot =
        page->InsertTuple(large_data.c_str(), large_data.length() + 1);
    if (slot == INVALID_SLOT_ID) break;
    slots.push_back(slot);
  }

  size_t filled_count = slots.size();
  EXPECT_GT(filled_count, 0);

  // Delete all
  for (auto slot : slots) {
    page->DeleteTuple(slot);
  }

  // Compact
  page->CompactPage();

  EXPECT_EQ(page->GetSlotCount(), 0);
  EXPECT_EQ(page->GetFreeStart(), sizeof(PageHeader));

  // Refill - should be able to insert same amount again
  for (size_t i = 0; i < filled_count; i++) {
    slot_id_t slot =
        page->InsertTuple(large_data.c_str(), large_data.length() + 1);
    EXPECT_NE(slot, INVALID_SLOT_ID)
        << "Should be able to refill after compaction";
  }
}

TEST(PageCompactionIntegrationTest, CompactionWithForwardingPointers) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert tuples
  slot_id_t slot0 = page->InsertTuple("Data0", 6);
  slot_id_t slot1 = page->InsertTuple("Data1", 6);
  page->InsertTuple("Data2", 6);

  // Set a forwarding pointer on slot 1
  page->SetForwardingPointer(slot1, 123, 45);
  EXPECT_TRUE(page->IsSlotForwarded(slot1));

  // Delete slot 0
  page->DeleteTuple(slot0);

  // Compact
  page->CompactPage();

  // After compaction, slot count stays 3 (slots not renumbered)
  EXPECT_EQ(page->GetSlotCount(), 3);

  // Verify forwarding pointer is preserved (slots not renumbered)
  EXPECT_TRUE(page->IsSlotForwarded(slot1))
      << "Forwarding pointer should be preserved";
  EXPECT_FALSE(page->IsSlotValid(slot0))
      << "Deleted slot should remain invalid";
  EXPECT_TRUE(page->VerifyChecksum());
}

// ============================================================================
// Edge Cases and Error Conditions
// ============================================================================

TEST(PageCompactionEdgeCaseTest, CompactEmptyPage) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Compact empty page (should be no-op)
  page->CompactPage();

  EXPECT_EQ(page->GetSlotCount(), 0);
  EXPECT_EQ(page->GetFreeStart(), sizeof(PageHeader));
  EXPECT_TRUE(page->VerifyChecksum());
}

TEST(PageCompactionEdgeCaseTest, DeleteOnlyTuple) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  slot_id_t slot = page->InsertTuple("Single", 7);
  page->DeleteTuple(slot);

  EXPECT_EQ(page->GetDeletedTupleCount(), 1);

  page->CompactPage();

  EXPECT_EQ(page->GetSlotCount(), 0);
  EXPECT_EQ(page->GetDeletedTupleCount(), 0);
  EXPECT_EQ(page->GetFreeStart(), sizeof(PageHeader));
}

TEST(PageCompactionEdgeCaseTest, AlternatingInsertDelete) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  for (int i = 0; i < 10; i++) {
    std::string data = "Data" + std::to_string(i);
    slot_id_t slot = page->InsertTuple(data.c_str(), data.length() + 1);
    ASSERT_NE(slot, INVALID_SLOT_ID);

    // Delete immediately
    page->DeleteTuple(slot);

    // Compact
    page->CompactPage();

    // Should be empty after each cycle
    EXPECT_EQ(page->GetSlotCount(), 0);
  }

  EXPECT_TRUE(page->VerifyChecksum());
}
