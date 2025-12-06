//
// Created by Abhi on 01/12/25.
//

#include "../include/page/page.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdlib>
#include <thread>
#include <vector>

#include "../include/common/types.h"

// Note: Tests now use 'delete page;' directly for cleanup
// Page::CreateNew() allocates memory that we're currently leaking
// In production code, use proper RAII or smart pointers

// Test createNew() functionality
TEST(PageTest, CreateNew) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr) << "Failed to create new page";

  // Check that page is initialized correctly
  EXPECT_EQ(page->GetPageId(), 0);
  EXPECT_EQ(page->GetSlotId(), 0);
  EXPECT_EQ(page->GetFreeStart(), sizeof(PageHeader));
  EXPECT_EQ(page->GetFreeEnd(), PAGE_SIZE);
  EXPECT_EQ(page->GetSlotCount(), 0);
  EXPECT_EQ(page->GetPageType(), 0);
  EXPECT_EQ(page->GetFlags(), 0);

  // Check that checksum is computed
  EXPECT_NE(page->GetChecksum(), 0)
      << "Checksum should be non-zero for initialized page";

  // Note: Page::CreateNew() allocates memory that needs to be freed
  // For now, we're leaking memory in tests - in production, use proper RAII
}

// Test that newly created page has valid checksum
TEST(PageTest, CreateNewValidChecksum) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  EXPECT_TRUE(page->VerifyChecksum())
      << "Newly created page should have valid checksum";
}

// Test checksum verification detects corruption
TEST(PageTest, ChecksumDetectsCorruption) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Verify initial checksum is valid
  EXPECT_TRUE(page->VerifyChecksum());

  // Corrupt the page by modifying data
  page->SetPageId(12345);

  // Checksum should now be invalid
  EXPECT_FALSE(page->VerifyChecksum())
      << "Modified page should have invalid checksum";
}

// Test checksum recalculation after modification
TEST(PageTest, RecalculateChecksumAfterModification) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Modify page data
  page->SetPageId(42);
  page->SetSlotCount(10);
  page->SetPageType(DATA_PAGE);

  // Recalculate checksum
  uint32_t newChecksum = page->ComputeChecksum();
  page->SetChecksum(newChecksum);

  // Verify checksum is now valid
  EXPECT_TRUE(page->VerifyChecksum())
      << "Checksum should be valid after recalculation";
}

// Test all getters return correct initial values
TEST(PageTest, GettersInitialValues) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  EXPECT_EQ(page->GetPageId(), 0);
  EXPECT_EQ(page->GetSlotId(), 0);
  EXPECT_EQ(page->GetFreeStart(), sizeof(PageHeader));
  EXPECT_EQ(page->GetFreeEnd(), PAGE_SIZE);
  EXPECT_EQ(page->GetSlotCount(), 0);
  EXPECT_EQ(page->GetPageType(), 0);
  EXPECT_EQ(page->GetFlags(), 0);
  EXPECT_NE(page->GetChecksum(), 0);
}

// Test all setters work correctly
TEST(PageTest, SettersWorkCorrectly) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  page->SetPageId(100);
  page->SetSlotId(50);
  page->SetFreeStart(200);
  page->SetFreeEnd(8000);
  page->SetSlotCount(25);
  page->SetPageType(INDEX_PAGE);
  page->SetFlags(0xFF);

  EXPECT_EQ(page->GetPageId(), 100);
  EXPECT_EQ(page->GetSlotId(), 50);
  EXPECT_EQ(page->GetFreeStart(), 200);
  EXPECT_EQ(page->GetFreeEnd(), 8000);
  EXPECT_EQ(page->GetSlotCount(), 25);
  EXPECT_EQ(page->GetPageType(), INDEX_PAGE);
  EXPECT_EQ(page->GetFlags(), 0xFF);
}

// Test getters with null buffer
TEST(PageTest, GettersWithNullHeader) {
  Page page{};
  // page_buffer_ is nullptr by default (constructor initializes it)

  EXPECT_EQ(page.GetPageId(), 0);
  EXPECT_EQ(page.GetSlotId(), 0);
  EXPECT_EQ(page.GetFreeStart(), 0);
  EXPECT_EQ(page.GetFreeEnd(), 0);
  EXPECT_EQ(page.GetSlotCount(), 0);
  EXPECT_EQ(page.GetPageType(), 0);
  EXPECT_EQ(page.GetFlags(), 0);
  EXPECT_EQ(page.GetChecksum(), 0);
}

// Test setters with null buffer (should not crash)
TEST(PageTest, SettersWithNullHeader) {
  Page page{};
  // page_buffer_ is nullptr by default

  EXPECT_NO_THROW({
    page.SetPageId(100);
    page.SetSlotId(50);
    page.SetFreeStart(200);
    page.SetFreeEnd(8000);
    page.SetSlotCount(25);
    page.SetPageType(1);
    page.SetFlags(0xFF);
    page.SetChecksum(12345);
  });
}

// Test verifyChecksum with null buffer
TEST(PageTest, VerifyChecksumWithNullHeader) {
  Page page{};
  // page_buffer_ is nullptr by default

  EXPECT_FALSE(page.VerifyChecksum())
      << "verifyChecksum should return false for null buffer";
}

// Test computeChecksum with null buffer
TEST(PageTest, ComputeChecksumWithNullHeader) {
  Page page{};
  // page_buffer_ is nullptr by default

  EXPECT_EQ(page.ComputeChecksum(), 0)
      << "computeChecksum should return 0 for null buffer";
}

// Test checksum consistency - same data produces same checksum
TEST(PageTest, ChecksumConsistency) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  uint32_t firstChecksum = page->ComputeChecksum();

  // Compute multiple times
  for (int i = 0; i < 10; i++) {
    uint32_t checksum = page->ComputeChecksum();
    EXPECT_EQ(checksum, firstChecksum)
        << "Checksum should be consistent on iteration " << i;
  }
}

// Test that different pages have different checksums
TEST(PageTest, DifferentPagesHaveDifferentChecksums) {
  auto page1 = Page::CreateNew();
  auto page2 = Page::CreateNew();
  ASSERT_NE(page1, nullptr);
  ASSERT_NE(page2, nullptr);

  page1->SetPageId(1);
  page1->SetChecksum(page1->ComputeChecksum());

  page2->SetPageId(2);
  page2->SetChecksum(page2->ComputeChecksum());

  EXPECT_NE(page1->GetChecksum(), page2->GetChecksum())
      << "Different pages should have different checksums";
}

// Test thread safety for verifyChecksum
TEST(PageTest, ThreadSafetyVerifyChecksum) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  std::vector<std::thread> threads;
  std::atomic<int> failureCount{0};

  // Multiple threads verifying checksum concurrently
  threads.reserve(10);
  for (int i = 0; i < 10; i++) {
    threads.emplace_back([&]() {
      for (int j = 0; j < 100; j++) {
        if (!page->VerifyChecksum()) {
          ++failureCount;
        }
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_EQ(failureCount, 0) << "No checksum verification should fail";
}

// Test thread safety for computeChecksum
TEST(PageTest, ThreadSafetyComputeChecksum) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  const uint32_t expectedChecksum = page->ComputeChecksum();
  std::vector<std::thread> threads;
  std::atomic<bool> failed{false};

  // Multiple threads computing checksum concurrently
  for (int i = 0; i < 10; i++) {
    threads.emplace_back([&]() {
      for (int j = 0; j < 100; j++) {
        uint32_t computed = page->ComputeChecksum();
        if (computed != expectedChecksum) {
          failed = true;
        }
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_FALSE(failed) << "All concurrent checksum computations should match";
}

// Test thread safety for getters
TEST(PageTest, ThreadSafetyGetters) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  page->SetPageId(42);
  page->SetSlotCount(10);

  std::vector<std::thread> threads;
  std::atomic<bool> failed{false};

  // Multiple threads reading concurrently
  threads.reserve(10);
  for (int i = 0; i < 10; i++) {
    threads.emplace_back([&]() {
      for (int j = 0; j < 1000; j++) {
        if (page->GetPageId() != 42 || page->GetSlotCount() != 10) {
          failed = true;
        }
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_FALSE(failed) << "Concurrent reads should be consistent";
}

// Test page type enums
TEST(PageTest, PageTypeEnums) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Test different page types
  page->SetPageType(DATA_PAGE);
  EXPECT_EQ(page->GetPageType(), DATA_PAGE);

  page->SetPageType(INDEX_PAGE);
  EXPECT_EQ(page->GetPageType(), INDEX_PAGE);

  page->SetPageType(FSM_PAGE);
  EXPECT_EQ(page->GetPageType(), FSM_PAGE);
}

// Test boundary values
TEST(PageTest, BoundaryValues) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  // Test max values for uint16_t
  page->SetPageId(0xFFFF);
  page->SetSlotId(0xFFFF);
  page->SetFreeStart(0xFFFF);
  page->SetFreeEnd(0xFFFF);
  page->SetSlotCount(0xFFFF);

  EXPECT_EQ(page->GetPageId(), 0xFFFF);
  EXPECT_EQ(page->GetSlotId(), 0xFFFF);
  EXPECT_EQ(page->GetFreeStart(), 0xFFFF);
  EXPECT_EQ(page->GetFreeEnd(), 0xFFFF);
  EXPECT_EQ(page->GetSlotCount(), 0xFFFF);

  // Test max value for uint8_t
  page->SetFlags(0xFF);
  EXPECT_EQ(page->GetFlags(), 0xFF);
}

// Test free space calculations
TEST(PageTest, FreeSpaceCalculations) {
  auto page = Page::CreateNew();
  ASSERT_NE(page, nullptr);

  const uint16_t freeStart = page->GetFreeStart();
  const uint16_t freeEnd = page->GetFreeEnd();

  EXPECT_LT(freeStart, freeEnd) << "Free start should be less than free end";

  size_t freeSpace = freeEnd - freeStart;
  EXPECT_GT(freeSpace, 0) << "Page should have free space";
  EXPECT_LE(freeSpace, PAGE_SIZE) << "Free space should not exceed page size";
}
