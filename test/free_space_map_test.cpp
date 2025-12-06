#include "../include/storage/free_space_map.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <filesystem>

class FreeSpaceMapTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create a unique test file for each test
    test_file_ = "test_fsm_" + std::to_string(test_counter_++) + ".fsm";
  }

  void TearDown() override {
    // Clean up test file
    if (std::filesystem::exists(test_file_)) {
      std::filesystem::remove(test_file_);
    }
  }

  std::string test_file_;
  static int test_counter_;
};

int FreeSpaceMapTest::test_counter_ = 0;

// Test 1: Basic initialization and file creation
TEST_F(FreeSpaceMapTest, InitializeCreatesFile) {
  FreeSpaceMap fsm(test_file_);

  ASSERT_TRUE(fsm.Initialize());
  EXPECT_TRUE(std::filesystem::exists(test_file_));
}

// Test 2: Category encoding matches specification
TEST_F(FreeSpaceMapTest, CategoryEncodingCorrect) {
  // Test formula: (available_bytes * 255) / 8192

  // No space
  EXPECT_EQ(FreeSpaceMap::BytesToCategory(0), 0);

  // Full page
  EXPECT_EQ(FreeSpaceMap::BytesToCategory(8192), 255);

  // Half page
  uint8_t half_category = FreeSpaceMap::BytesToCategory(4096);
  EXPECT_GE(half_category, 126);
  EXPECT_LE(half_category, 128);

  // Quarter page
  uint8_t quarter_category = FreeSpaceMap::BytesToCategory(2048);
  EXPECT_GE(quarter_category, 63);
  EXPECT_LE(quarter_category, 64);

  // Specific values
  EXPECT_EQ(FreeSpaceMap::BytesToCategory(1000), (1000 * 255) / 8192);
  EXPECT_EQ(FreeSpaceMap::BytesToCategory(5000), (5000 * 255) / 8192);
}

// Test 3: Category to bytes reverse conversion
TEST_F(FreeSpaceMapTest, CategoryToBytesConversion) {
  EXPECT_EQ(FreeSpaceMap::CategoryToBytes(0), 0);
  EXPECT_EQ(FreeSpaceMap::CategoryToBytes(255), 8192);

  // Test round-trip conversion
  for (uint16_t bytes = 0; bytes <= 8192; bytes += 100) {
    uint8_t category = FreeSpaceMap::BytesToCategory(bytes);
    uint16_t converted_back = FreeSpaceMap::CategoryToBytes(category);

    // Allow some rounding error
    int diff =
        std::abs(static_cast<int>(converted_back) - static_cast<int>(bytes));
    EXPECT_LT(diff, 50) << "Bytes: " << bytes
                        << ", Category: " << static_cast<int>(category)
                        << ", Back: " << converted_back;
  }
}

// Test 4: Update and retrieve page free space
TEST_F(FreeSpaceMapTest, UpdateAndGetPageFreeSpace) {
  FreeSpaceMap fsm(test_file_);
  ASSERT_TRUE(fsm.Initialize());

  // Update page 0 with 4096 bytes free
  fsm.UpdatePageFreeSpace(0, 4096);
  uint8_t category = fsm.GetCategory(0);
  uint8_t expected = FreeSpaceMap::BytesToCategory(4096);
  EXPECT_EQ(category, expected);

  // Update page 5 with 1024 bytes free
  fsm.UpdatePageFreeSpace(5, 1024);
  category = fsm.GetCategory(5);
  expected = FreeSpaceMap::BytesToCategory(1024);
  EXPECT_EQ(category, expected);

  // Verify page 0 is still correct
  EXPECT_EQ(fsm.GetCategory(0), FreeSpaceMap::BytesToCategory(4096));
}

// Test 5: FindPageWithSpace returns correct page
TEST_F(FreeSpaceMapTest, FindPageWithSpaceReturnsCorrectPage) {
  FreeSpaceMap fsm(test_file_);
  ASSERT_TRUE(fsm.Initialize());

  // Create pages with different amounts of free space
  fsm.UpdatePageFreeSpace(0, 100);   // Very little space
  fsm.UpdatePageFreeSpace(1, 500);   // Small space
  fsm.UpdatePageFreeSpace(2, 2000);  // Moderate space
  fsm.UpdatePageFreeSpace(3, 4000);  // Large space
  fsm.UpdatePageFreeSpace(4, 8000);  // Almost full space

  // Find page with at least 1000 bytes - should return a valid page
  // Note: Order is non-deterministic (unordered_set), so just verify we get a
  // valid page
  page_id_t found = fsm.FindPageWithSpace(1000);
  EXPECT_NE(found, INVALID_PAGE_ID);
  EXPECT_GE(FreeSpaceMap::CategoryToBytes(fsm.GetCategory(found)),
            968);  // Allow rounding

  // Find page with at least 3000 bytes - should return a valid page (3 or 4)
  found = fsm.FindPageWithSpace(3000);
  EXPECT_NE(found, INVALID_PAGE_ID);
  EXPECT_GE(FreeSpaceMap::CategoryToBytes(fsm.GetCategory(found)), 2968);

  // Find page with at least 100 bytes - should return a valid page
  found = fsm.FindPageWithSpace(100);
  EXPECT_NE(found, INVALID_PAGE_ID);
  EXPECT_GE(FreeSpaceMap::CategoryToBytes(fsm.GetCategory(found)), 68);

  // Find page with at least 8100 bytes - should return INVALID_PAGE_ID
  found = fsm.FindPageWithSpace(8100);
  EXPECT_EQ(found, INVALID_PAGE_ID);
}

// Test 6: FindPageWithSpace with no suitable page
TEST_F(FreeSpaceMapTest, FindPageWithSpaceNoSuitablePage) {
  FreeSpaceMap fsm(test_file_);
  ASSERT_TRUE(fsm.Initialize());

  // Create pages with little space
  fsm.UpdatePageFreeSpace(0, 100);
  fsm.UpdatePageFreeSpace(1, 200);
  fsm.UpdatePageFreeSpace(2, 300);

  // Try to find page with 1000 bytes
  page_id_t found = fsm.FindPageWithSpace(1000);
  EXPECT_EQ(found, INVALID_PAGE_ID);
}

// Test 7: Persistence - data survives across instances
TEST_F(FreeSpaceMapTest, DataPersistsAcrossInstances) {
  // Create FSM and add data
  {
    FreeSpaceMap fsm(test_file_);
    ASSERT_TRUE(fsm.Initialize());

    fsm.UpdatePageFreeSpace(0, 1000);
    fsm.UpdatePageFreeSpace(1, 2000);
    fsm.UpdatePageFreeSpace(2, 3000);

    ASSERT_TRUE(fsm.Flush());
  }

  // Create new FSM instance and verify data
  {
    FreeSpaceMap fsm(test_file_);
    ASSERT_TRUE(fsm.Initialize());

    EXPECT_EQ(fsm.GetCategory(0), FreeSpaceMap::BytesToCategory(1000));
    EXPECT_EQ(fsm.GetCategory(1), FreeSpaceMap::BytesToCategory(2000));
    EXPECT_EQ(fsm.GetCategory(2), FreeSpaceMap::BytesToCategory(3000));
  }
}

// Test 8: Test with 100 pages
TEST_F(FreeSpaceMapTest, Test100Pages) {
  FreeSpaceMap fsm(test_file_);
  ASSERT_TRUE(fsm.Initialize());

  // Add 100 pages with varying free space
  for (page_id_t i = 0; i < 100; i++) {
    uint16_t free_space = (i * 80) % 8192;  // Varying pattern
    fsm.UpdatePageFreeSpace(i, free_space);
  }

  // Verify all pages
  for (page_id_t i = 0; i < 100; i++) {
    uint16_t free_space = (i * 80) % 8192;
    uint8_t expected_category = FreeSpaceMap::BytesToCategory(free_space);
    EXPECT_EQ(fsm.GetCategory(i), expected_category);
  }

  // Test persistence
  ASSERT_TRUE(fsm.Flush());

  // Reload and verify
  FreeSpaceMap fsm2(test_file_);
  ASSERT_TRUE(fsm2.Initialize());

  for (page_id_t i = 0; i < 100; i++) {
    uint16_t free_space = (i * 80) % 8192;
    uint8_t expected_category = FreeSpaceMap::BytesToCategory(free_space);
    EXPECT_EQ(fsm2.GetCategory(i), expected_category);
  }
}

// Test 9: Test with 1000 pages
TEST_F(FreeSpaceMapTest, Test1000Pages) {
  FreeSpaceMap fsm(test_file_);
  ASSERT_TRUE(fsm.Initialize());

  // Add 1000 pages
  for (page_id_t i = 0; i < 1000; i++) {
    uint16_t free_space = (i * 8) % 8192;
    fsm.UpdatePageFreeSpace(i, free_space);
  }

  EXPECT_EQ(fsm.GetPageCount(), 1000);

  // Spot check some pages
  EXPECT_EQ(fsm.GetCategory(0), FreeSpaceMap::BytesToCategory(0));
  EXPECT_EQ(fsm.GetCategory(500),
            FreeSpaceMap::BytesToCategory((500 * 8) % 8192));
  EXPECT_EQ(fsm.GetCategory(999),
            FreeSpaceMap::BytesToCategory((999 * 8) % 8192));

  // Test find operation
  page_id_t found = fsm.FindPageWithSpace(4000);
  EXPECT_NE(found, INVALID_PAGE_ID);
  // Allow for rounding error in category encoding (up to ~32 bytes)
  uint16_t found_bytes = FreeSpaceMap::CategoryToBytes(fsm.GetCategory(found));
  EXPECT_GE(found_bytes, 3968);  // 4000 - 32 (tolerance for rounding)
}

// Test 10: Test with 10000 pages and performance
TEST_F(FreeSpaceMapTest, Test10000PagesWithPerformance) {
  FreeSpaceMap fsm(test_file_);
  ASSERT_TRUE(fsm.Initialize());

  // Add 10000 pages
  for (page_id_t i = 0; i < 10000; i++) {
    uint16_t free_space = (i * 3) % 8192;
    fsm.UpdatePageFreeSpace(i, free_space);
  }

  EXPECT_EQ(fsm.GetPageCount(), 10000);

  // Performance test: FindPageWithSpace should be < 1ms
  auto start = std::chrono::high_resolution_clock::now();

  for (int iteration = 0; iteration < 1000; iteration++) {
    uint16_t required = 1000 + (iteration % 5000);
    page_id_t found = fsm.FindPageWithSpace(required);
    (void)found;  // Suppress unused variable warning
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  double avg_time_us = duration.count() / 1000.0;

  std::cout << "Average FindPageWithSpace time for 10K pages: " << avg_time_us
            << " microseconds" << std::endl;

  // Should be well under 1ms (1000 microseconds)
  EXPECT_LT(avg_time_us, 1000.0)
      << "FindPageWithSpace took " << avg_time_us << " us (expected < 1000 us)";
}

// Test 11: SetCategory method
TEST_F(FreeSpaceMapTest, SetCategoryDirectly) {
  FreeSpaceMap fsm(test_file_);
  ASSERT_TRUE(fsm.Initialize());

  fsm.SetCategory(0, 100);
  EXPECT_EQ(fsm.GetCategory(0), 100);

  fsm.SetCategory(5, 255);
  EXPECT_EQ(fsm.GetCategory(5), 255);

  fsm.SetCategory(10, 0);
  EXPECT_EQ(fsm.GetCategory(10), 0);
}

// Test 12: Page count tracking
TEST_F(FreeSpaceMapTest, PageCountTracking) {
  FreeSpaceMap fsm(test_file_);
  ASSERT_TRUE(fsm.Initialize());

  EXPECT_EQ(fsm.GetPageCount(), 0);

  fsm.UpdatePageFreeSpace(0, 1000);
  EXPECT_EQ(fsm.GetPageCount(), 1);

  fsm.UpdatePageFreeSpace(5, 2000);
  EXPECT_EQ(fsm.GetPageCount(), 6);

  fsm.UpdatePageFreeSpace(100, 3000);
  EXPECT_EQ(fsm.GetPageCount(), 101);
}

// Test 13: Empty FSM behavior
TEST_F(FreeSpaceMapTest, EmptyFSMBehavior) {
  FreeSpaceMap fsm(test_file_);
  ASSERT_TRUE(fsm.Initialize());

  // FindPageWithSpace on empty FSM should return INVALID_PAGE_ID
  EXPECT_EQ(fsm.FindPageWithSpace(1000), INVALID_PAGE_ID);

  // GetCategory on non-existent page should return 0
  EXPECT_EQ(fsm.GetCategory(0), 0);
  EXPECT_EQ(fsm.GetCategory(100), 0);
}

// Test 14: Boundary values
TEST_F(FreeSpaceMapTest, BoundaryValues) {
  FreeSpaceMap fsm(test_file_);
  ASSERT_TRUE(fsm.Initialize());

  // Test with 0 bytes
  fsm.UpdatePageFreeSpace(0, 0);
  EXPECT_EQ(fsm.GetCategory(0), 0);

  // Test with maximum bytes (8192)
  fsm.UpdatePageFreeSpace(1, 8192);
  EXPECT_EQ(fsm.GetCategory(1), 255);

  // Test with bytes > 8192 (should clamp to 8192)
  fsm.UpdatePageFreeSpace(2, 10000);
  EXPECT_EQ(fsm.GetCategory(2), 255);
}

// Test 15: Multiple flushes
TEST_F(FreeSpaceMapTest, MultipleFlushes) {
  FreeSpaceMap fsm(test_file_);
  ASSERT_TRUE(fsm.Initialize());

  // First batch
  fsm.UpdatePageFreeSpace(0, 1000);
  fsm.UpdatePageFreeSpace(1, 2000);
  ASSERT_TRUE(fsm.Flush());

  // Second batch
  fsm.UpdatePageFreeSpace(2, 3000);
  fsm.UpdatePageFreeSpace(3, 4000);
  ASSERT_TRUE(fsm.Flush());

  // Reload and verify all data
  FreeSpaceMap fsm2(test_file_);
  ASSERT_TRUE(fsm2.Initialize());

  EXPECT_EQ(fsm2.GetCategory(0), FreeSpaceMap::BytesToCategory(1000));
  EXPECT_EQ(fsm2.GetCategory(1), FreeSpaceMap::BytesToCategory(2000));
  EXPECT_EQ(fsm2.GetCategory(2), FreeSpaceMap::BytesToCategory(3000));
  EXPECT_EQ(fsm2.GetCategory(3), FreeSpaceMap::BytesToCategory(4000));
}

// Test 16: Destructor flushes dirty data
TEST_F(FreeSpaceMapTest, DestructorFlushesDirtyData) {
  // Create FSM and add data without explicit flush
  {
    FreeSpaceMap fsm(test_file_);
    ASSERT_TRUE(fsm.Initialize());

    fsm.UpdatePageFreeSpace(0, 1500);
    fsm.UpdatePageFreeSpace(1, 2500);
    // Destructor should flush automatically
  }

  // Reload and verify
  FreeSpaceMap fsm(test_file_);
  ASSERT_TRUE(fsm.Initialize());

  EXPECT_EQ(fsm.GetCategory(0), FreeSpaceMap::BytesToCategory(1500));
  EXPECT_EQ(fsm.GetCategory(1), FreeSpaceMap::BytesToCategory(2500));
}

// Test 17: Non-sequential page allocation (sparse pages)
TEST_F(FreeSpaceMapTest, NonSequentialPageAllocation) {
  FreeSpaceMap fsm(test_file_);
  ASSERT_TRUE(fsm.Initialize());

  // Allocate pages non-sequentially: 0, 5, 17, 100, 200
  fsm.UpdatePageFreeSpace(0, 1000);
  fsm.UpdatePageFreeSpace(5, 2000);
  fsm.UpdatePageFreeSpace(17, 3000);
  fsm.UpdatePageFreeSpace(100, 4000);
  fsm.UpdatePageFreeSpace(200, 5000);

  // Verify GetCategory only returns values for allocated pages
  EXPECT_GT(fsm.GetCategory(0), 0);
  EXPECT_GT(fsm.GetCategory(5), 0);
  EXPECT_GT(fsm.GetCategory(17), 0);
  EXPECT_GT(fsm.GetCategory(100), 0);
  EXPECT_GT(fsm.GetCategory(200), 0);

  // Non-allocated pages should return 0
  EXPECT_EQ(fsm.GetCategory(1), 0);
  EXPECT_EQ(fsm.GetCategory(10), 0);
  EXPECT_EQ(fsm.GetCategory(50), 0);
  EXPECT_EQ(fsm.GetCategory(150), 0);

  // FindPageWithSpace should only return allocated pages
  page_id_t found = fsm.FindPageWithSpace(2500);
  EXPECT_NE(found, INVALID_PAGE_ID);
  // Should be one of the allocated pages
  bool is_allocated =
      (found == 0 || found == 5 || found == 17 || found == 100 || found == 200);
  EXPECT_TRUE(is_allocated);

  // Verify persistence
  ASSERT_TRUE(fsm.Flush());

  // Reload and verify
  FreeSpaceMap fsm2(test_file_);
  ASSERT_TRUE(fsm2.Initialize());

  EXPECT_GT(fsm2.GetCategory(0), 0);
  EXPECT_GT(fsm2.GetCategory(5), 0);
  EXPECT_GT(fsm2.GetCategory(17), 0);
  EXPECT_GT(fsm2.GetCategory(100), 0);
  EXPECT_GT(fsm2.GetCategory(200), 0);

  // Non-allocated pages still return 0
  EXPECT_EQ(fsm2.GetCategory(1), 0);
  EXPECT_EQ(fsm2.GetCategory(10), 0);
}

// Test 18: Realistic workload simulation
TEST_F(FreeSpaceMapTest, RealisticWorkloadSimulation) {
  FreeSpaceMap fsm(test_file_);
  ASSERT_TRUE(fsm.Initialize());

  // Simulate inserting tuples into pages
  // Initially, all pages are empty
  for (page_id_t i = 0; i < 50; i++) {
    fsm.UpdatePageFreeSpace(i, 8192);  // Empty pages
  }

  // Simulate tuple insertions
  for (int insertion = 0; insertion < 1000; insertion++) {
    // Find page with space for 200 bytes
    page_id_t page = fsm.FindPageWithSpace(200);

    if (page == INVALID_PAGE_ID) {
      // Allocate new page
      page = fsm.GetPageCount();
      fsm.UpdatePageFreeSpace(page, 8192);
    }

    // Simulate insertion - reduce free space
    uint16_t current_free =
        FreeSpaceMap::CategoryToBytes(fsm.GetCategory(page));
    uint16_t new_free = current_free - 200;
    fsm.UpdatePageFreeSpace(page, new_free);
  }

  // Verify we have pages with varying free space
  int full_pages = 0;
  int partial_pages = 0;
  int empty_pages = 0;

  for (page_id_t i = 0; i < fsm.GetPageCount(); i++) {
    uint8_t category = fsm.GetCategory(i);
    if (category == 0) {
      full_pages++;
    } else if (category == 255) {
      empty_pages++;
    } else {
      partial_pages++;
    }
  }

  std::cout << "Workload result - Full: " << full_pages
            << ", Partial: " << partial_pages << ", Empty: " << empty_pages
            << std::endl;

  // We should have some partially filled pages
  EXPECT_GT(partial_pages, 0);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
