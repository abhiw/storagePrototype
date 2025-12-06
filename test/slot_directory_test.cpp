//
// Created by Abhi on 01/12/25.
//

#include <gtest/gtest.h>

#include <cstdlib>

#include "../include/common/types.h"
#include "../include/page/page.h"

// Test adding a single slot
TEST(SlotDirectoryTest, AddSingleSlot) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Add a slot at offset 100 with length 50
  slot_id_t slot_id = page->AddSlot(100, 50);
  EXPECT_EQ(slot_id, 0) << "First slot should have ID 0";
  EXPECT_EQ(page->GetSlotCount(), 1) << "Slot count should be 1";

  // Verify slot entry
  SlotEntry& slot_entry = page->GetSlotEntry(slot_id);
  EXPECT_EQ(slot_entry.offset, 100);
  EXPECT_EQ(slot_entry.length, 50);
  EXPECT_TRUE(page->IsSlotValid(slot_id));
}

// Test adding multiple slots
TEST(SlotDirectoryTest, AddMultipleSlots) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Add 10 slots
  for (int i = 0; i < 10; i++) {
    slot_id_t slot_id = page->AddSlot(100 + i * 10, 10);
    EXPECT_EQ(slot_id, i) << "Slot ID should match iteration count";
  }

  EXPECT_EQ(page->GetSlotCount(), 10) << "Should have 10 slots";

  // Verify all slots
  for (int i = 0; i < 10; i++) {
    EXPECT_TRUE(page->IsSlotValid(i));
    SlotEntry& slot_entry = page->GetSlotEntry(i);
    EXPECT_EQ(slot_entry.offset, 100 + i * 10);
    EXPECT_EQ(slot_entry.length, 10);
  }
}

// Test slot entry is at correct offset
TEST(SlotDirectoryTest, SlotEntryOffset) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Add first slot
  slot_id_t slot0 = page->AddSlot(100, 50);
  EXPECT_EQ(slot0, 0);

  // First slot (slot 0) should be at offset: 8192 - (0+1)*8 = 8184
  auto* page_data = reinterpret_cast<uint8_t*>(page->GetRawBuffer());
  SlotEntry* slot0_ptr = reinterpret_cast<SlotEntry*>(page_data + 8184);
  EXPECT_EQ(slot0_ptr->offset, 100);
  EXPECT_EQ(slot0_ptr->length, 50);

  // Add second slot
  slot_id_t slot1 = page->AddSlot(200, 60);
  EXPECT_EQ(slot1, 1);

  // Second slot (slot 1) should be at offset: 8192 - (1+1)*8 = 8176
  SlotEntry* slot1_ptr = reinterpret_cast<SlotEntry*>(page_data + 8176);
  EXPECT_EQ(slot1_ptr->offset, 200);
  EXPECT_EQ(slot1_ptr->length, 60);
}

// Test marking slot as deleted
TEST(SlotDirectoryTest, MarkSlotDeleted) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Add slots
  slot_id_t slot0 = page->AddSlot(100, 50);
  slot_id_t slot1 = page->AddSlot(200, 60);

  // Initially both should be valid
  EXPECT_TRUE(page->IsSlotValid(slot0));
  EXPECT_TRUE(page->IsSlotValid(slot1));

  // Mark slot 0 as deleted
  page->MarkSlotDeleted(slot0);
  EXPECT_FALSE(page->IsSlotValid(slot0)) << "Deleted slot should be invalid";
  EXPECT_TRUE(page->IsSlotValid(slot1)) << "Other slot should remain valid";
}

// Test forwarding pointer encoding/decoding
TEST(SlotDirectoryTest, ForwardingPointerEncodingDecoding) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Add a slot
  slot_id_t slot_id = page->AddSlot(100, 50);

  // Initially not forwarded
  EXPECT_FALSE(page->IsSlotForwarded(slot_id));

  // Set forwarding pointer to page 1234, slot 42
  page->SetForwardingPointer(slot_id, 1234, 42);

  // Should now be marked as forwarded
  EXPECT_TRUE(page->IsSlotForwarded(slot_id));

  // Decode forwarding pointer
  TupleId forward = page->GetForwardingPointer(slot_id);
  EXPECT_EQ(forward.page_id, 1234) << "Page ID should match";
  EXPECT_EQ(forward.slot_id, 42) << "Slot ID should match";
}

// Test forwarding pointer with max values
TEST(SlotDirectoryTest, ForwardingPointerMaxValues) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  slot_id_t slot_id = page->AddSlot(100, 50);

  // Test max page_id (16 bits = 65535)
  page->SetForwardingPointer(slot_id, 65535, 255);
  TupleId forward = page->GetForwardingPointer(slot_id);
  EXPECT_EQ(forward.page_id, 65535);
  EXPECT_EQ(forward.slot_id, 255);

  // Test another combination
  page->SetForwardingPointer(slot_id, 12345, 123);
  forward = page->GetForwardingPointer(slot_id);
  EXPECT_EQ(forward.page_id, 12345);
  EXPECT_EQ(forward.slot_id, 123);
}

// Test adding up to 1000 slots
TEST(SlotDirectoryTest, Add1000Slots) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Add 1000 slots
  int addedSlots = 0;
  for (int i = 0; i < 1000; i++) {
    slot_id_t slot_id = page->AddSlot(sizeof(PageHeader) + i, 1);
    if (slot_id == static_cast<slot_id_t>(-1)) {
      // Page is full
      break;
    }
    addedSlots++;
    EXPECT_EQ(slot_id, i);
  }

  EXPECT_GE(addedSlots, 1000) << "Should be able to add at least 1000 slots";
  EXPECT_EQ(page->GetSlotCount(), addedSlots);

  // Verify all added slots are valid
  for (int i = 0; i < addedSlots; i++) {
    EXPECT_TRUE(page->IsSlotValid(i)) << "Slot " << i << " should be valid";
  }
}

// Test boundary case: first slot
TEST(SlotDirectoryTest, FirstSlot) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  slot_id_t first_slot = page->AddSlot(sizeof(PageHeader), 100);
  EXPECT_EQ(first_slot, 0);
  EXPECT_TRUE(page->IsSlotValid(first_slot));

  SlotEntry& entry = page->GetSlotEntry(first_slot);
  EXPECT_EQ(entry.offset, sizeof(PageHeader));
  EXPECT_EQ(entry.length, 100);
  EXPECT_EQ(entry.flags & SLOT_VALID, SLOT_VALID);
}

// Test boundary case: checking free space
TEST(SlotDirectoryTest, FreeSpaceManagement) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  uint16_t initialFreeEnd = page->GetFreeEnd();
  EXPECT_EQ(initialFreeEnd, PAGE_SIZE);

  // Add first slot
  page->AddSlot(100, 50);
  uint16_t afterFirstSlot = page->GetFreeEnd();
  EXPECT_EQ(afterFirstSlot, PAGE_SIZE - SLOT_ENTRY_SIZE);

  // Add second slot
  page->AddSlot(200, 60);
  uint16_t afterSecondSlot = page->GetFreeEnd();
  EXPECT_EQ(afterSecondSlot, PAGE_SIZE - 2 * SLOT_ENTRY_SIZE);
}

// Test slot entry size is exactly 8 bytes
TEST(SlotDirectoryTest, SlotEntrySizeIs8Bytes) {
  EXPECT_EQ(sizeof(SlotEntry), 8) << "SlotEntry must be exactly 8 bytes";
  EXPECT_EQ(SLOT_ENTRY_SIZE, 8);
}

// Test slot flags
TEST(SlotDirectoryTest, SlotFlags) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  slot_id_t slot_id = page->AddSlot(100, 50);
  SlotEntry& entry = page->GetSlotEntry(slot_id);

  // Initially should only have VALID flag
  EXPECT_TRUE(entry.flags & SLOT_VALID);
  EXPECT_FALSE(entry.flags & SLOT_FORWARDED);
  EXPECT_FALSE(entry.flags & SLOT_COMPRESSED);

  // Set forwarding pointer
  page->SetForwardingPointer(slot_id, 1, 2);
  EXPECT_TRUE(entry.flags & SLOT_VALID);
  EXPECT_TRUE(entry.flags & SLOT_FORWARDED);

  // Mark as deleted
  page->MarkSlotDeleted(slot_id);
  EXPECT_FALSE(entry.flags & SLOT_VALID);
  EXPECT_TRUE(entry.flags & SLOT_FORWARDED);  // FORWARDED flag should remain
}

// Test invalid slot operations
TEST(SlotDirectoryTest, InvalidSlotOperations) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Add one slot
  page->AddSlot(100, 50);

  // Try to access slot that doesn't exist
  EXPECT_FALSE(page->IsSlotValid(999));
  EXPECT_FALSE(page->IsSlotForwarded(999));

  // Mark non-existent slot as deleted (should not crash)
  EXPECT_NO_THROW(page->MarkSlotDeleted(999));

  // Get forwarding pointer for non-existent slot
  TupleId forward = page->GetForwardingPointer(999);
  EXPECT_EQ(forward.page_id, 0);
  EXPECT_EQ(forward.slot_id, 0);
}

// Test slot operations with null header
TEST(SlotDirectoryTest, NullHeaderHandling) {
  Page page;
  // page_buffer_ is nullptr by default (constructor initializes it)

  // All operations should handle null gracefully
  EXPECT_EQ(page.AddSlot(100, 50), static_cast<slot_id_t>(-1));
  EXPECT_EQ(page.GetSlotCount(), 0);
  EXPECT_FALSE(page.IsSlotValid(0));
  EXPECT_FALSE(page.IsSlotForwarded(0));
  EXPECT_NO_THROW(page.MarkSlotDeleted(0));

  TupleId forward = page.GetForwardingPointer(0);
  EXPECT_EQ(forward.page_id, 0);
  EXPECT_EQ(forward.slot_id, 0);
}

// Test slot reuse after deletion
TEST(SlotDirectoryTest, SlotReuseAfterDeletion) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Add 3 slots
  slot_id_t slot0 = page->AddSlot(100, 50);
  slot_id_t slot1 = page->AddSlot(200, 60);
  slot_id_t slot2 = page->AddSlot(300, 70);

  EXPECT_EQ(page->GetSlotCount(), 3);

  // Mark slot 1 as deleted
  page->MarkSlotDeleted(slot1);
  EXPECT_FALSE(page->IsSlotValid(slot1));

  // Slot count should remain 3 (slots are not removed, just marked invalid)
  EXPECT_EQ(page->GetSlotCount(), 3);

  // Other slots should still be valid
  EXPECT_TRUE(page->IsSlotValid(slot0));
  EXPECT_TRUE(page->IsSlotValid(slot2));
}

// Test page becomes full
TEST(SlotDirectoryTest, PageFull) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Calculate max slots that can fit
  // Available space = PAGE_SIZE - sizeof(PageHeader)
  // Each slot takes SLOT_ENTRY_SIZE in the directory
  size_t maxSlots = (PAGE_SIZE - sizeof(PageHeader)) / SLOT_ENTRY_SIZE;

  // Add slots until page is full
  for (size_t i = 0; i < maxSlots + 10; i++) {
    slot_id_t slot_id = page->AddSlot(sizeof(PageHeader), 0);
    if (slot_id == static_cast<slot_id_t>(-1)) {
      // Page is full
      break;
    }
  }

  // Try to add one more slot - should fail
  slot_id_t overflow = page->AddSlot(100, 50);
  EXPECT_EQ(overflow, static_cast<slot_id_t>(-1))
      << "Adding to full page should fail";
}

// Test slot directory grows downward correctly
TEST(SlotDirectoryTest, SlotDirectoryGrowsDownward) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  auto* pageData = reinterpret_cast<uint8_t*>(page->GetRawBuffer());

  // Add slots and verify they're placed correctly
  for (int i = 0; i < 5; i++) {
    page->AddSlot(100 + i, 10 + i);

    // Slot i should be at offset: PAGE_SIZE - (i+1) * SLOT_ENTRY_SIZE
    size_t expectedOffset = PAGE_SIZE - (i + 1) * SLOT_ENTRY_SIZE;
    SlotEntry* slotPtr =
        reinterpret_cast<SlotEntry*>(pageData + expectedOffset);

    EXPECT_EQ(slotPtr->offset, 100 + i) << "Slot " << i << " offset mismatch";
    EXPECT_EQ(slotPtr->length, 10 + i) << "Slot " << i << " length mismatch";
  }

  // Verify free_End pointer
  size_t expectedFreeEnd = PAGE_SIZE - 5 * SLOT_ENTRY_SIZE;
  EXPECT_EQ(page->GetFreeEnd(), expectedFreeEnd);
}

// Test forwarding chain
TEST(SlotDirectoryTest, ForwardingChain) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Create a chain: slot 0 -> page 10, slot 5 -> page 20, slot 15
  slot_id_t slot0 = page->AddSlot(100, 50);
  slot_id_t slot1 = page->AddSlot(200, 60);
  slot_id_t slot2 = page->AddSlot(300, 70);

  // Set forwarding pointers
  page->SetForwardingPointer(slot0, 10, 5);
  page->SetForwardingPointer(slot1, 20, 15);

  // Verify chain
  TupleId forward0 = page->GetForwardingPointer(slot0);
  EXPECT_EQ(forward0.page_id, 10);
  EXPECT_EQ(forward0.slot_id, 5);

  TupleId forward1 = page->GetForwardingPointer(slot1);
  EXPECT_EQ(forward1.page_id, 20);
  EXPECT_EQ(forward1.slot_id, 15);

  // Slot 2 should not be forwarded
  EXPECT_FALSE(page->IsSlotForwarded(slot2));
}
