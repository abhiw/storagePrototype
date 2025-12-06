//
// Page Insertion Test Suite
// Tests for P2.T2: Implement Page Insertion Algorithm
//

#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <thread>
#include <vector>

#include "../include/common/config.h"
#include "../include/common/types.h"
#include "../include/page/page.h"

// Helper function to create test tuple data
std::vector<char> CreateTestTuple(uint16_t size, uint8_t fill_byte = 0xAB) {
  std::vector<char> data(size);
  std::memset(data.data(), fill_byte, size);
  return data;
}

// ============================================================================
// A. Basic Functionality Tests
// ============================================================================

TEST(PageInsertionTest, InsertSingleTuple) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Create test tuple
  const uint16_t tuple_size = 100;
  auto tuple_data = CreateTestTuple(tuple_size);

  // Insert tuple
  slot_id_t slot_id = page->InsertTuple(tuple_data.data(), tuple_size);

  // Verify insertion succeeded
  EXPECT_EQ(slot_id, 0) << "First tuple should get slot_id 0";
  EXPECT_TRUE(page->IsSlotValid(slot_id)) << "Slot should be marked as valid";

  // Verify slot entry details
  SlotEntry& slot_entry = page->GetSlotEntry(slot_id);
  EXPECT_EQ(slot_entry.offset, sizeof(PageHeader))
      << "First tuple should be after header";
  EXPECT_EQ(slot_entry.length, tuple_size)
      << "Slot length should match tuple size";
  EXPECT_EQ(slot_entry.flags & SLOT_VALID, SLOT_VALID)
      << "VALID flag should be set";

  // Verify free space decreased correctly
  const size_t expected_free_space =
      PAGE_SIZE - sizeof(PageHeader) - tuple_size - SLOT_ENTRY_SIZE;
  EXPECT_EQ(page->GetAvailableFreeSpace(), expected_free_space);

  // Verify checksum is still valid
  EXPECT_TRUE(page->VerifyChecksum())
      << "Checksum should be valid after insertion";
}

TEST(PageInsertionTest, InsertMultipleTuples) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert 10 tuples of varying sizes
  std::vector<uint16_t> tuple_sizes = {50, 100, 75, 200, 125,
                                       80, 150, 60, 90,  110};
  std::vector<slot_id_t> slot_ids;

  for (size_t i = 0; i < tuple_sizes.size(); ++i) {
    auto tuple_data = CreateTestTuple(tuple_sizes[i], static_cast<uint8_t>(i));
    slot_id_t slot_id = page->InsertTuple(tuple_data.data(), tuple_sizes[i]);

    EXPECT_NE(slot_id, INVALID_SLOT_ID)
        << "Insertion " << i << " should succeed";
    EXPECT_EQ(slot_id, i) << "Slot IDs should be sequential";
    EXPECT_TRUE(page->IsSlotValid(slot_id))
        << "Slot " << slot_id << " should be valid";

    slot_ids.push_back(slot_id);
  }

  // Verify slot count
  EXPECT_EQ(page->GetSlotCount(), tuple_sizes.size());

  // Verify all slots are valid
  for (slot_id_t slot_id : slot_ids) {
    EXPECT_TRUE(page->IsSlotValid(slot_id));
  }

  // Verify checksum
  EXPECT_TRUE(page->VerifyChecksum());
}

TEST(PageInsertionTest, InsertUntilFull) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  const uint16_t tuple_size = 100;
  auto tuple_data = CreateTestTuple(tuple_size);

  int successful_insertions = 0;
  slot_id_t last_valid_slot = INVALID_SLOT_ID;

  // Keep inserting until we fail
  while (true) {
    slot_id_t slot_id = page->InsertTuple(tuple_data.data(), tuple_size);

    if (slot_id == INVALID_SLOT_ID) {
      // Page is full
      break;
    }

    successful_insertions++;
    last_valid_slot = slot_id;
    EXPECT_TRUE(page->IsSlotValid(slot_id));
  }

  EXPECT_GT(successful_insertions, 0)
      << "Should be able to insert at least one tuple";
  EXPECT_NE(last_valid_slot, INVALID_SLOT_ID);

  // Verify we cannot insert even 1 more byte
  auto tiny_tuple = CreateTestTuple(1);
  slot_id_t slot_id = page->InsertTuple(tiny_tuple.data(), 1);
  EXPECT_EQ(slot_id, INVALID_SLOT_ID)
      << "Should not be able to insert more data";

  // Verify free space is minimal (< minimum tuple size + slot entry)
  EXPECT_LT(page->GetAvailableFreeSpace(), tuple_size + SLOT_ENTRY_SIZE);

  // Verify checksum still valid
  EXPECT_TRUE(page->VerifyChecksum());
}

// ============================================================================
// B. Slot Reuse Tests
// ============================================================================

TEST(PageInsertionTest, ReuseDeletedSlot) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert 5 tuples
  const uint16_t tuple_size = 50;
  for (int i = 0; i < 5; ++i) {
    auto tuple_data = CreateTestTuple(tuple_size, static_cast<uint8_t>(i));
    page->InsertTuple(tuple_data.data(), tuple_size);
  }

  EXPECT_EQ(page->GetSlotCount(), 5);

  // Delete tuple at slot_id 2
  page->MarkSlotDeleted(2);
  EXPECT_FALSE(page->IsSlotValid(2)) << "Slot 2 should be marked as deleted";

  // Insert a new tuple - should reuse slot 2
  auto new_tuple = CreateTestTuple(tuple_size, 0xFF);
  slot_id_t slot_id = page->InsertTuple(new_tuple.data(), tuple_size);

  EXPECT_EQ(slot_id, 2) << "Should reuse slot 2";
  EXPECT_TRUE(page->IsSlotValid(2)) << "Slot 2 should now be valid again";
  EXPECT_EQ(page->GetSlotCount(), 5)
      << "Slot count should remain 5 (not increment to 6)";

  // Verify checksum
  EXPECT_TRUE(page->VerifyChecksum());
}

TEST(PageInsertionTest, ReuseMultipleDeletedSlots) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert 10 tuples
  const uint16_t tuple_size = 50;
  for (int i = 0; i < 10; ++i) {
    auto tuple_data = CreateTestTuple(tuple_size, static_cast<uint8_t>(i));
    page->InsertTuple(tuple_data.data(), tuple_size);
  }

  // Delete slots 1, 3, 5, 7 (alternating pattern)
  std::vector<slot_id_t> deleted_slots = {1, 3, 5, 7};
  for (slot_id_t slot : deleted_slots) {
    page->MarkSlotDeleted(slot);
    EXPECT_FALSE(page->IsSlotValid(slot));
  }

  // Insert 4 new tuples - should reuse deleted slots in order
  std::vector<slot_id_t> reused_slots;
  for (int i = 0; i < 4; ++i) {
    auto tuple_data = CreateTestTuple(tuple_size, 0xCC);
    slot_id_t slot_id = page->InsertTuple(tuple_data.data(), tuple_size);
    EXPECT_NE(slot_id, INVALID_SLOT_ID);
    reused_slots.push_back(slot_id);
  }

  // Verify all deleted slots were reused
  EXPECT_EQ(reused_slots, deleted_slots);

  // Verify slot count remains 10
  EXPECT_EQ(page->GetSlotCount(), 10);

  // Verify all reused slots are now valid
  for (slot_id_t slot : reused_slots) {
    EXPECT_TRUE(page->IsSlotValid(slot));
  }
}

TEST(PageInsertionTest, ReuseAfterDeletingAllSlots) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert 5 tuples
  const uint16_t tuple_size = 50;
  for (int i = 0; i < 5; ++i) {
    auto tuple_data = CreateTestTuple(tuple_size, static_cast<uint8_t>(i));
    page->InsertTuple(tuple_data.data(), tuple_size);
  }

  // Delete all 5 tuples
  for (slot_id_t i = 0; i < 5; ++i) {
    page->MarkSlotDeleted(i);
  }

  // Verify all slots are invalid
  for (slot_id_t i = 0; i < 5; ++i) {
    EXPECT_FALSE(page->IsSlotValid(i));
  }

  // Insert 3 new tuples
  std::vector<slot_id_t> new_slots;
  for (int i = 0; i < 3; ++i) {
    auto tuple_data = CreateTestTuple(tuple_size, 0xDD);
    slot_id_t slot_id = page->InsertTuple(tuple_data.data(), tuple_size);
    new_slots.push_back(slot_id);
  }

  // Verify slots 0, 1, 2 were reused
  EXPECT_EQ(new_slots[0], 0);
  EXPECT_EQ(new_slots[1], 1);
  EXPECT_EQ(new_slots[2], 2);

  // Verify slots 3, 4 remain deleted
  EXPECT_FALSE(page->IsSlotValid(3));
  EXPECT_FALSE(page->IsSlotValid(4));

  // Slot count should still be 5
  EXPECT_EQ(page->GetSlotCount(), 5);
}

// ============================================================================
// C. Space Management Tests
// ============================================================================

TEST(PageInsertionTest, ExactFitTuple) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Fill page partially with medium tuples
  const uint16_t medium_tuple_size = 500;
  while (page->GetAvailableFreeSpace() > 1000) {
    auto tuple_data = CreateTestTuple(medium_tuple_size);
    slot_id_t slot_id = page->InsertTuple(tuple_data.data(), medium_tuple_size);
    if (slot_id == INVALID_SLOT_ID) {
      break;
    }
  }

  // Calculate exact remaining space
  size_t available = page->GetAvailableFreeSpace();

  // Try to insert tuple that exactly fits (minus slot entry for new slot)
  if (available >= SLOT_ENTRY_SIZE + 1) {
    uint16_t exact_size = available - SLOT_ENTRY_SIZE;
    auto exact_tuple = CreateTestTuple(exact_size);

    slot_id_t slot_id = page->InsertTuple(exact_tuple.data(), exact_size);
    EXPECT_NE(slot_id, INVALID_SLOT_ID)
        << "Should be able to insert exact-fit tuple";

    // Verify very little space remains
    EXPECT_LT(page->GetAvailableFreeSpace(), SLOT_ENTRY_SIZE);

    // Next insertion should fail
    auto tiny_tuple = CreateTestTuple(1);
    slot_id_t next_slot = page->InsertTuple(tiny_tuple.data(), 1);
    EXPECT_EQ(next_slot, INVALID_SLOT_ID);
  }
}

TEST(PageInsertionTest, TupleTooLarge) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Fill page to about 75% capacity
  const uint16_t tuple_size = 200;
  while (page->GetAvailableFreeSpace() > PAGE_SIZE / 4) {
    auto tuple_data = CreateTestTuple(tuple_size);
    page->InsertTuple(tuple_data.data(), tuple_size);
  }

  // Try to insert tuple larger than remaining space
  size_t available = page->GetAvailableFreeSpace();
  uint16_t too_large_size = available + 100;
  auto large_tuple = CreateTestTuple(too_large_size);

  slot_id_t slot_id = page->InsertTuple(large_tuple.data(), too_large_size);

  EXPECT_EQ(slot_id, INVALID_SLOT_ID)
      << "Should not be able to insert tuple that's too large";

  // Verify page state unchanged
  EXPECT_EQ(page->GetAvailableFreeSpace(), available);
  EXPECT_TRUE(page->VerifyChecksum());
}

TEST(PageInsertionTest, VariableSizeTuples) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Insert tuples of various sizes
  std::vector<uint16_t> sizes = {10, 100, 1000, 50, 500};

  for (uint16_t size : sizes) {
    auto tuple_data = CreateTestTuple(size);
    slot_id_t slot_id = page->InsertTuple(tuple_data.data(), size);

    EXPECT_NE(slot_id, INVALID_SLOT_ID)
        << "Should be able to insert tuple of size " << size;

    // Verify slot entry has correct length
    SlotEntry& entry = page->GetSlotEntry(slot_id);
    EXPECT_EQ(entry.length, size);
  }

  // Verify all tuples are valid
  for (slot_id_t i = 0; i < sizes.size(); ++i) {
    EXPECT_TRUE(page->IsSlotValid(i));
  }

  EXPECT_TRUE(page->VerifyChecksum());
}

// ============================================================================
// D. Checksum Integrity Tests
// ============================================================================

TEST(PageInsertionTest, ChecksumValidAfterInsertion) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  const uint16_t tuple_size = 100;

  // Insert 20 tuples and verify checksum after each
  for (int i = 0; i < 20; ++i) {
    auto tuple_data = CreateTestTuple(tuple_size, static_cast<uint8_t>(i));
    slot_id_t slot_id = page->InsertTuple(tuple_data.data(), tuple_size);

    EXPECT_NE(slot_id, INVALID_SLOT_ID);
    EXPECT_TRUE(page->VerifyChecksum())
        << "Checksum should be valid after insertion " << i;
  }
}

TEST(PageInsertionTest, ChecksumChangesWithInsertion) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  uint32_t previous_checksum = page->GetChecksum();
  const uint16_t tuple_size = 50;

  for (int i = 0; i < 5; ++i) {
    auto tuple_data = CreateTestTuple(tuple_size, static_cast<uint8_t>(i));
    slot_id_t slot_id = page->InsertTuple(tuple_data.data(), tuple_size);

    EXPECT_NE(slot_id, INVALID_SLOT_ID);

    uint32_t new_checksum = page->GetChecksum();
    EXPECT_NE(new_checksum, previous_checksum)
        << "Checksum should change after insertion " << i;
    EXPECT_TRUE(page->VerifyChecksum());

    previous_checksum = new_checksum;
  }
}

// ============================================================================
// E. Edge Cases Tests
// ============================================================================

TEST(PageInsertionTest, InsertZeroSizeTuple) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  char dummy_data = 'A';
  slot_id_t slot_id = page->InsertTuple(&dummy_data, 0);

  EXPECT_EQ(slot_id, INVALID_SLOT_ID)
      << "Should not be able to insert zero-size tuple";
  EXPECT_EQ(page->GetSlotCount(), 0) << "Slot count should remain 0";
  EXPECT_TRUE(page->VerifyChecksum());
}

TEST(PageInsertionTest, InsertNullTupleData) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  slot_id_t slot_id = page->InsertTuple(nullptr, 100);

  EXPECT_EQ(slot_id, INVALID_SLOT_ID)
      << "Should not be able to insert null data";
  EXPECT_EQ(page->GetSlotCount(), 0);
  EXPECT_TRUE(page->VerifyChecksum());
}

TEST(PageInsertionTest, InsertOnNullPage) {
  Page page;  // Default constructor, page_buffer_ is nullptr

  auto tuple_data = CreateTestTuple(100);
  slot_id_t slot_id = page.InsertTuple(tuple_data.data(), 100);

  EXPECT_EQ(slot_id, INVALID_SLOT_ID)
      << "Should fail gracefully with null buffer";
}

// ============================================================================
// F. Stress Tests
// ============================================================================

TEST(PageInsertionTest, Insert1000SmallTuples) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  const uint16_t tuple_size = 4;
  int successful_insertions = 0;

  for (int i = 0; i < 1000; ++i) {
    auto tuple_data =
        CreateTestTuple(tuple_size, static_cast<uint8_t>(i & 0xFF));
    slot_id_t slot_id = page->InsertTuple(tuple_data.data(), tuple_size);

    if (slot_id != INVALID_SLOT_ID) {
      successful_insertions++;
    } else {
      // Page is full
      break;
    }
  }

  EXPECT_GT(successful_insertions, 0);
  EXPECT_EQ(page->GetSlotCount(), successful_insertions);
  EXPECT_TRUE(page->VerifyChecksum());

  // Verify all inserted slots are valid
  for (slot_id_t i = 0; i < successful_insertions; ++i) {
    EXPECT_TRUE(page->IsSlotValid(i));
  }
}

TEST(PageInsertionTest, InsertDeleteInsertPattern) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  const uint16_t tuple_size = 50;

  // Phase 1: Insert 100 tuples
  for (int i = 0; i < 100; ++i) {
    auto tuple_data = CreateTestTuple(tuple_size, static_cast<uint8_t>(i));
    slot_id_t slot_id = page->InsertTuple(tuple_data.data(), tuple_size);
    if (slot_id == INVALID_SLOT_ID) break;
  }

  int initial_count = page->GetSlotCount();
  EXPECT_GT(initial_count, 0);

  // Phase 2: Delete every other tuple (50 deletions)
  for (slot_id_t i = 0; i < initial_count; i += 2) {
    page->MarkSlotDeleted(i);
  }

  // Count valid slots
  int valid_after_delete = 0;
  for (slot_id_t i = 0; i < initial_count; ++i) {
    if (page->IsSlotValid(i)) {
      valid_after_delete++;
    }
  }

  // Phase 3: Insert more tuples (should reuse deleted slots)
  int reinsertions = 0;
  for (int i = 0; i < initial_count / 2; ++i) {
    auto tuple_data = CreateTestTuple(tuple_size, 0xEE);
    slot_id_t slot_id = page->InsertTuple(tuple_data.data(), tuple_size);
    if (slot_id != INVALID_SLOT_ID) {
      reinsertions++;
    } else {
      break;
    }
  }

  // Verify slot count hasn't grown beyond initial
  EXPECT_LE(page->GetSlotCount(), initial_count);

  // Final valid count should be close to initial
  int final_valid = 0;
  for (slot_id_t i = 0; i < page->GetSlotCount(); ++i) {
    if (page->IsSlotValid(i)) {
      final_valid++;
    }
  }

  EXPECT_GE(final_valid, valid_after_delete);
  EXPECT_TRUE(page->VerifyChecksum());
}

TEST(PageInsertionTest, FreeSpaceTrackingAccuracy) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  size_t initial_free_space = page->GetAvailableFreeSpace();
  EXPECT_GT(initial_free_space, 0);

  const uint16_t tuple_size = 100;
  auto tuple_data = CreateTestTuple(tuple_size);

  // Insert tuple and verify free space decreased correctly
  slot_id_t slot_id = page->InsertTuple(tuple_data.data(), tuple_size);
  EXPECT_NE(slot_id, INVALID_SLOT_ID);

  size_t expected_free_space =
      initial_free_space - tuple_size - SLOT_ENTRY_SIZE;
  size_t actual_free_space = page->GetAvailableFreeSpace();

  EXPECT_EQ(actual_free_space, expected_free_space)
      << "Free space should decrease by tuple_size + SLOT_ENTRY_SIZE";

  // Insert another and verify again
  slot_id = page->InsertTuple(tuple_data.data(), tuple_size);
  EXPECT_NE(slot_id, INVALID_SLOT_ID);

  expected_free_space -= (tuple_size + SLOT_ENTRY_SIZE);
  actual_free_space = page->GetAvailableFreeSpace();

  EXPECT_EQ(actual_free_space, expected_free_space);
}
