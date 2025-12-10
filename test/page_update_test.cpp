#include <gtest/gtest.h>

#include <cstring>
#include <vector>

#include "../include/common/types.h"
#include "../include/page/page.h"

// ============================================================================
// UpdateTupleInPlace Tests
// ============================================================================

TEST(PageUpdateTest, UpdateTupleInPlace_Success) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert initial tuple
  const char* initial_data = "Hello, World!";
  uint16_t initial_size = strlen(initial_data);
  slot_id_t slot_id = page->InsertTuple(initial_data, initial_size);
  ASSERT_NE(slot_id, INVALID_SLOT_ID);

  // Update with smaller data (should fit)
  const char* new_data = "Hello!";
  uint16_t new_size = strlen(new_data);
  ErrorCode result = page->UpdateTupleInPlace(slot_id, new_data, new_size);

  EXPECT_EQ(result.code, 0);
  EXPECT_TRUE(page->VerifyChecksum());
  EXPECT_TRUE(page->IsDirty());
}

TEST(PageUpdateTest, UpdateTupleInPlace_SameSize) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert initial tuple
  const char* initial_data = "Test Data 123";
  uint16_t initial_size = strlen(initial_data);
  slot_id_t slot_id = page->InsertTuple(initial_data, initial_size);
  ASSERT_NE(slot_id, INVALID_SLOT_ID);

  // Update with same size data
  const char* new_data = "New Data  456";
  uint16_t new_size = strlen(new_data);
  ASSERT_EQ(new_size, initial_size);

  ErrorCode result = page->UpdateTupleInPlace(slot_id, new_data, new_size);

  EXPECT_EQ(result.code, 0);
  EXPECT_TRUE(page->VerifyChecksum());
}

TEST(PageUpdateTest, UpdateTupleInPlace_SmallerSize) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert initial tuple
  const char* initial_data = "This is a longer string";
  uint16_t initial_size = strlen(initial_data);
  slot_id_t slot_id = page->InsertTuple(initial_data, initial_size);
  ASSERT_NE(slot_id, INVALID_SLOT_ID);

  // Update with smaller data
  const char* new_data = "Short";
  uint16_t new_size = strlen(new_data);
  ASSERT_LT(new_size, initial_size);

  ErrorCode result = page->UpdateTupleInPlace(slot_id, new_data, new_size);

  EXPECT_EQ(result.code, 0);
  EXPECT_TRUE(page->VerifyChecksum());
}

TEST(PageUpdateTest, UpdateTupleInPlace_LargerSize_Fails) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert initial tuple
  const char* initial_data = "Short";
  uint16_t initial_size = strlen(initial_data);
  slot_id_t slot_id = page->InsertTuple(initial_data, initial_size);
  ASSERT_NE(slot_id, INVALID_SLOT_ID);

  // Try to update with larger data (should fail)
  const char* new_data = "This is a much longer string";
  uint16_t new_size = strlen(new_data);
  ASSERT_GT(new_size, initial_size);

  ErrorCode result = page->UpdateTupleInPlace(slot_id, new_data, new_size);

  EXPECT_EQ(result.code, -8);  // Size exceeds current size
}

TEST(PageUpdateTest, UpdateTupleInPlace_InvalidSlot) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  const char* data = "Test";
  uint16_t size = strlen(data);

  // Try to update non-existent slot
  ErrorCode result = page->UpdateTupleInPlace(100, data, size);

  EXPECT_EQ(result.code, -4);  // Invalid slot id
}

TEST(PageUpdateTest, UpdateTupleInPlace_DeletedSlot) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert and then delete tuple
  const char* initial_data = "Test";
  slot_id_t slot_id = page->InsertTuple(initial_data, strlen(initial_data));
  ASSERT_NE(slot_id, INVALID_SLOT_ID);

  page->DeleteTuple(slot_id);

  // Try to update deleted slot
  const char* new_data = "New";
  ErrorCode result =
      page->UpdateTupleInPlace(slot_id, new_data, strlen(new_data));

  EXPECT_EQ(result.code, -6);  // Slot is not valid
}

TEST(PageUpdateTest, UpdateTupleInPlace_ForwardedSlot) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert two tuples
  const char* data1 = "Tuple 1";
  const char* data2 = "Tuple 2";
  slot_id_t slot_id1 = page->InsertTuple(data1, strlen(data1));
  slot_id_t slot_id2 = page->InsertTuple(data2, strlen(data2));
  ASSERT_NE(slot_id1, INVALID_SLOT_ID);
  ASSERT_NE(slot_id2, INVALID_SLOT_ID);

  // Mark first slot as forwarded to second
  ErrorCode mark_result =
      page->MarkSlotForwarded(slot_id1, page->GetPageId(), slot_id2);
  ASSERT_EQ(mark_result.code, 0);

  // Try to update forwarded slot
  const char* new_data = "New Data";
  ErrorCode result =
      page->UpdateTupleInPlace(slot_id1, new_data, strlen(new_data));

  EXPECT_EQ(result.code, -7);  // Slot is forwarded
}

TEST(PageUpdateTest, UpdateTupleInPlace_NullData) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert tuple
  const char* initial_data = "Test";
  slot_id_t slot_id = page->InsertTuple(initial_data, strlen(initial_data));
  ASSERT_NE(slot_id, INVALID_SLOT_ID);

  // Try to update with null data
  ErrorCode result = page->UpdateTupleInPlace(slot_id, nullptr, 10);

  EXPECT_EQ(result.code, -2);  // New data is null
}

TEST(PageUpdateTest, UpdateTupleInPlace_ZeroSize) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert tuple
  const char* initial_data = "Test";
  slot_id_t slot_id = page->InsertTuple(initial_data, strlen(initial_data));
  ASSERT_NE(slot_id, INVALID_SLOT_ID);

  // Try to update with zero size
  const char* new_data = "Test";
  ErrorCode result = page->UpdateTupleInPlace(slot_id, new_data, 0);

  EXPECT_EQ(result.code, -3);  // New size is zero
}

// ============================================================================
// MarkSlotForwarded Tests
// ============================================================================

TEST(PageUpdateTest, MarkSlotForwarded_Success) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);
  page->SetPageId(1);

  // Insert tuple
  const char* data = "Test Tuple";
  slot_id_t slot_id = page->InsertTuple(data, strlen(data));
  ASSERT_NE(slot_id, INVALID_SLOT_ID);

  // Mark as forwarded
  page_id_t target_page = 5;
  slot_id_t target_slot = 10;
  ErrorCode result = page->MarkSlotForwarded(slot_id, target_page, target_slot);

  EXPECT_EQ(result.code, 0);
  EXPECT_TRUE(page->IsSlotForwarded(slot_id));
  EXPECT_TRUE(page->IsDirty());

  // Verify forwarding pointer
  TupleId fwd_ptr = page->GetForwardingPointer(slot_id);
  EXPECT_EQ(fwd_ptr.page_id, target_page);
  EXPECT_EQ(fwd_ptr.slot_id, target_slot);
}

TEST(PageUpdateTest, MarkSlotForwarded_EncodingDecoding) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);
  page->SetPageId(1);

  // Insert tuple
  const char* data = "Test";
  slot_id_t slot_id = page->InsertTuple(data, strlen(data));
  ASSERT_NE(slot_id, INVALID_SLOT_ID);

  // Test various page_id and slot_id combinations
  struct TestCase {
    page_id_t page_id;
    slot_id_t slot_id;
  };

  std::vector<TestCase> test_cases = {
      {0, 0},      {1, 1},       {255, 42},
      {1234, 100}, {65535, 255},  // Max values that fit in 16-bit + 8-bit
  };

  for (const auto& tc : test_cases) {
    // Insert new tuple for each test case
    slot_id_t test_slot = page->InsertTuple(data, strlen(data));
    ASSERT_NE(test_slot, INVALID_SLOT_ID);

    ErrorCode result =
        page->MarkSlotForwarded(test_slot, tc.page_id, tc.slot_id);
    ASSERT_EQ(result.code, 0);

    TupleId fwd_ptr = page->GetForwardingPointer(test_slot);
    EXPECT_EQ(fwd_ptr.page_id, tc.page_id)
        << "Failed for page_id=" << tc.page_id;
    EXPECT_EQ(fwd_ptr.slot_id, tc.slot_id)
        << "Failed for slot_id=" << tc.slot_id;
  }
}

TEST(PageUpdateTest, MarkSlotForwarded_InvalidSlot) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Try to mark non-existent slot as forwarded
  ErrorCode result = page->MarkSlotForwarded(100, 5, 10);

  EXPECT_EQ(result.code, -2);  // Invalid slot id
}

TEST(PageUpdateTest, MarkSlotForwarded_DeletedSlot) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert and delete tuple
  const char* data = "Test";
  slot_id_t slot_id = page->InsertTuple(data, strlen(data));
  ASSERT_NE(slot_id, INVALID_SLOT_ID);

  page->DeleteTuple(slot_id);

  // Try to mark deleted slot as forwarded
  ErrorCode result = page->MarkSlotForwarded(slot_id, 5, 10);

  EXPECT_EQ(result.code, -4);  // Slot is not valid
}

// ============================================================================
// FollowForwardingChain Tests
// ============================================================================

TEST(PageUpdateTest, FollowForwardingChain_NoForwarding) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);
  page->SetPageId(1);

  // Insert tuple without forwarding
  const char* data = "Test Tuple";
  slot_id_t slot_id = page->InsertTuple(data, strlen(data));
  ASSERT_NE(slot_id, INVALID_SLOT_ID);

  // Follow chain - should return the same slot
  TupleId result = page->FollowForwardingChain(slot_id);

  EXPECT_EQ(result.page_id, page->GetPageId());
  EXPECT_EQ(result.slot_id, slot_id);
}

TEST(PageUpdateTest, FollowForwardingChain_SingleHop) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);
  page->SetPageId(1);

  // Insert two tuples
  const char* data = "Test";
  slot_id_t slot_id1 = page->InsertTuple(data, strlen(data));
  slot_id_t slot_id2 = page->InsertTuple(data, strlen(data));
  ASSERT_NE(slot_id1, INVALID_SLOT_ID);
  ASSERT_NE(slot_id2, INVALID_SLOT_ID);

  // Mark first as forwarded to second
  ErrorCode mark_result =
      page->MarkSlotForwarded(slot_id1, page->GetPageId(), slot_id2);
  ASSERT_EQ(mark_result.code, 0);

  // Follow chain - should end at slot_id2
  TupleId result = page->FollowForwardingChain(slot_id1);

  EXPECT_EQ(result.page_id, page->GetPageId());
  EXPECT_EQ(result.slot_id, slot_id2);
}

TEST(PageUpdateTest, FollowForwardingChain_MultipleHops) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);
  page->SetPageId(1);

  // Insert 5 tuples
  const char* data = "Test";
  std::vector<slot_id_t> slots;
  for (int i = 0; i < 5; i++) {
    slot_id_t slot = page->InsertTuple(data, strlen(data));
    ASSERT_NE(slot, INVALID_SLOT_ID);
    slots.push_back(slot);
  }

  // Create chain: 0 -> 1 -> 2 -> 3 -> 4
  for (size_t i = 0; i < slots.size() - 1; i++) {
    ErrorCode result =
        page->MarkSlotForwarded(slots[i], page->GetPageId(), slots[i + 1]);
    ASSERT_EQ(result.code, 0);
  }

  // Follow chain from slot 0 - should end at slot 4
  TupleId result = page->FollowForwardingChain(slots[0]);

  EXPECT_EQ(result.page_id, page->GetPageId());
  EXPECT_EQ(result.slot_id, slots[4]);
}

TEST(PageUpdateTest, FollowForwardingChain_ExactlyTenHops) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);
  page->SetPageId(1);

  // Insert 11 tuples (chain of length 10)
  const char* data = "T";
  std::vector<slot_id_t> slots;
  for (int i = 0; i < 11; i++) {
    slot_id_t slot = page->InsertTuple(data, strlen(data));
    ASSERT_NE(slot, INVALID_SLOT_ID);
    slots.push_back(slot);
  }

  // Create chain: 0 -> 1 -> 2 -> ... -> 10
  for (size_t i = 0; i < slots.size() - 1; i++) {
    ErrorCode result =
        page->MarkSlotForwarded(slots[i], page->GetPageId(), slots[i + 1]);
    ASSERT_EQ(result.code, 0);
  }

  // Follow chain with max_hops=10 - should succeed
  TupleId result = page->FollowForwardingChain(slots[0], 10);

  EXPECT_EQ(result.page_id, page->GetPageId());
  EXPECT_EQ(result.slot_id, slots[10]);
}

TEST(PageUpdateTest, FollowForwardingChain_ExceedsMaxHops) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);
  page->SetPageId(1);

  // Insert 12 tuples (chain of length 11, exceeds default max of 10)
  const char* data = "T";
  std::vector<slot_id_t> slots;
  for (int i = 0; i < 12; i++) {
    slot_id_t slot = page->InsertTuple(data, strlen(data));
    ASSERT_NE(slot, INVALID_SLOT_ID);
    slots.push_back(slot);
  }

  // Create chain: 0 -> 1 -> 2 -> ... -> 11
  for (size_t i = 0; i < slots.size() - 1; i++) {
    ErrorCode result =
        page->MarkSlotForwarded(slots[i], page->GetPageId(), slots[i + 1]);
    ASSERT_EQ(result.code, 0);
  }

  // Follow chain with max_hops=10 - should fail (returns 0,0)
  TupleId result = page->FollowForwardingChain(slots[0], 10);

  EXPECT_EQ(result.page_id, 0);
  EXPECT_EQ(result.slot_id, 0);
}

TEST(PageUpdateTest, FollowForwardingChain_CircularChain_TwoSlots) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);
  page->SetPageId(1);

  // Insert two tuples
  const char* data = "Test";
  slot_id_t slot_id1 = page->InsertTuple(data, strlen(data));
  slot_id_t slot_id2 = page->InsertTuple(data, strlen(data));
  ASSERT_NE(slot_id1, INVALID_SLOT_ID);
  ASSERT_NE(slot_id2, INVALID_SLOT_ID);

  // Create circular chain: 0 -> 1, 1 -> 0
  page->MarkSlotForwarded(slot_id1, page->GetPageId(), slot_id2);

  // Manually set forwarding on slot_id2 back to slot_id1 to create cycle
  page->SetForwardingPointer(slot_id2, page->GetPageId(), slot_id1);
  SlotEntry& entry = page->GetSlotEntry(slot_id2);
  entry.flags |= SLOT_FORWARDED;

  // Follow chain - should detect circular reference and return 0,0
  TupleId result = page->FollowForwardingChain(slot_id1);

  EXPECT_EQ(result.page_id, 0);
  EXPECT_EQ(result.slot_id, 0);
}

TEST(PageUpdateTest, FollowForwardingChain_CircularChain_ThreeSlots) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);
  page->SetPageId(1);

  // Insert three tuples
  const char* data = "T";
  std::vector<slot_id_t> slots;
  for (int i = 0; i < 3; i++) {
    slot_id_t slot = page->InsertTuple(data, strlen(data));
    ASSERT_NE(slot, INVALID_SLOT_ID);
    slots.push_back(slot);
  }

  // Create circular chain: 0 -> 1 -> 2 -> 0
  page->MarkSlotForwarded(slots[0], page->GetPageId(), slots[1]);
  page->MarkSlotForwarded(slots[1], page->GetPageId(), slots[2]);

  // Manually set forwarding on slot 2 back to slot 0 to create cycle
  page->SetForwardingPointer(slots[2], page->GetPageId(), slots[0]);
  SlotEntry& entry = page->GetSlotEntry(slots[2]);
  entry.flags |= SLOT_FORWARDED;

  // Follow chain - should detect circular reference
  TupleId result = page->FollowForwardingChain(slots[0]);

  EXPECT_EQ(result.page_id, 0);
  EXPECT_EQ(result.slot_id, 0);
}

TEST(PageUpdateTest, FollowForwardingChain_CrossPage) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);
  page->SetPageId(1);

  // Insert tuple
  const char* data = "Test";
  slot_id_t slot_id = page->InsertTuple(data, strlen(data));
  ASSERT_NE(slot_id, INVALID_SLOT_ID);

  // Mark as forwarded to different page
  page_id_t target_page = 5;
  slot_id_t target_slot = 10;
  ErrorCode result = page->MarkSlotForwarded(slot_id, target_page, target_slot);
  ASSERT_EQ(result.code, 0);

  // Follow chain - should return the target page/slot
  TupleId chain_result = page->FollowForwardingChain(slot_id);

  EXPECT_EQ(chain_result.page_id, target_page);
  EXPECT_EQ(chain_result.slot_id, target_slot);
}

TEST(PageUpdateTest, FollowForwardingChain_InvalidSlot) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Try to follow chain for non-existent slot
  TupleId result = page->FollowForwardingChain(100);

  EXPECT_EQ(result.page_id, 0);
  EXPECT_EQ(result.slot_id, 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(PageUpdateTest, Integration_UpdateAndForward) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);
  page->SetPageId(1);

  // Insert tuple
  const char* data1 = "Original Data";
  slot_id_t slot_id1 = page->InsertTuple(data1, strlen(data1));
  ASSERT_NE(slot_id1, INVALID_SLOT_ID);

  // Update in place
  const char* data2 = "Updated";
  ErrorCode update_result =
      page->UpdateTupleInPlace(slot_id1, data2, strlen(data2));
  ASSERT_EQ(update_result.code, 0);

  // Insert another tuple
  const char* data3 = "New Location";
  slot_id_t slot_id2 = page->InsertTuple(data3, strlen(data3));
  ASSERT_NE(slot_id2, INVALID_SLOT_ID);

  // Now mark first slot as forwarded
  ErrorCode forward_result =
      page->MarkSlotForwarded(slot_id1, page->GetPageId(), slot_id2);
  ASSERT_EQ(forward_result.code, 0);

  // Follow the chain
  TupleId chain_result = page->FollowForwardingChain(slot_id1);
  EXPECT_EQ(chain_result.page_id, page->GetPageId());
  EXPECT_EQ(chain_result.slot_id, slot_id2);

  // Verify checksum is still valid
  EXPECT_TRUE(page->VerifyChecksum());
}

TEST(PageUpdateTest, Integration_ComplexForwardingChain) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);
  page->SetPageId(1);

  // Create a complex scenario with 10 tuples
  const char* data = "D";
  std::vector<slot_id_t> slots;
  for (int i = 0; i < 10; i++) {
    slot_id_t slot = page->InsertTuple(data, strlen(data));
    ASSERT_NE(slot, INVALID_SLOT_ID);
    slots.push_back(slot);
  }

  // Create chains:
  // Chain 1: 0 -> 2 -> 4 -> 6 (length 3)
  // Chain 2: 1 -> 3 -> 5 (length 2)
  // Standalone: 7, 8, 9
  page->MarkSlotForwarded(slots[0], page->GetPageId(), slots[2]);
  page->MarkSlotForwarded(slots[2], page->GetPageId(), slots[4]);
  page->MarkSlotForwarded(slots[4], page->GetPageId(), slots[6]);

  page->MarkSlotForwarded(slots[1], page->GetPageId(), slots[3]);
  page->MarkSlotForwarded(slots[3], page->GetPageId(), slots[5]);

  // Test Chain 1
  TupleId result1 = page->FollowForwardingChain(slots[0]);
  EXPECT_EQ(result1.page_id, page->GetPageId());
  EXPECT_EQ(result1.slot_id, slots[6]);

  // Test Chain 2
  TupleId result2 = page->FollowForwardingChain(slots[1]);
  EXPECT_EQ(result2.page_id, page->GetPageId());
  EXPECT_EQ(result2.slot_id, slots[5]);

  // Test standalone slots
  TupleId result3 = page->FollowForwardingChain(slots[7]);
  EXPECT_EQ(result3.page_id, page->GetPageId());
  EXPECT_EQ(result3.slot_id, slots[7]);

  EXPECT_TRUE(page->VerifyChecksum());
}
