#include "../include/storage/disk_manager.h"

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <thread>
#include <vector>

#include "../include/page/page.h"

namespace fs = std::filesystem;

class DiskManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Ensure /tmp/test directory exists
    fs::create_directories("/tmp/test");
    test_db_file_ =
        "/tmp/test/test_db_" + std::to_string(GetTestId()) + ".db";
    CleanupTestFile();
  }

  void TearDown() override { CleanupTestFile(); }

  void CleanupTestFile() {
    if (fs::exists(test_db_file_)) {
      fs::remove(test_db_file_);
    }
  }

  uint64_t GetTestId() {
    return std::chrono::system_clock::now().time_since_epoch().count();
  }

  // Helper function to get PageHeader from a buffer
  PageHeader* GetHeaderFromBuffer(char* buffer) {
    return reinterpret_cast<PageHeader*>(buffer);
  }

  std::string test_db_file_;
};

// Test: Creating a new database file
TEST_F(DiskManagerTest, CreateNewDatabase) {
  EXPECT_FALSE(fs::exists(test_db_file_));

  {
    DiskManager disk_manager(test_db_file_);
    EXPECT_TRUE(disk_manager.IsOpen());
    EXPECT_TRUE(fs::exists(test_db_file_));
  }

  // File should still exist after destructor
  EXPECT_TRUE(fs::exists(test_db_file_));
}

// Test: Opening an existing database file
TEST_F(DiskManagerTest, OpenExistingDatabase) {
  // Create a database first
  {
    DiskManager disk_manager1(test_db_file_);
    EXPECT_TRUE(disk_manager1.IsOpen());
  }

  // Open it again
  {
    DiskManager disk_manager2(test_db_file_);
    EXPECT_TRUE(disk_manager2.IsOpen());
  }
}

// Test: IsOpen functionality
TEST_F(DiskManagerTest, IsOpenCheck) {
  DiskManager disk_manager(test_db_file_);
  EXPECT_TRUE(disk_manager.IsOpen());
}

// Test: Allocate single page
TEST_F(DiskManagerTest, AllocateSinglePage) {
  DiskManager disk_manager(test_db_file_);

  page_id_t page_id = disk_manager.AllocatePage();
  EXPECT_EQ(page_id, 1);

  page_id_t page_id2 = disk_manager.AllocatePage();
  EXPECT_EQ(page_id2, 2);
}

// Test: Allocate multiple pages
TEST_F(DiskManagerTest, AllocateMultiplePages) {
  DiskManager disk_manager(test_db_file_);

  std::vector<page_id_t> allocated_pages;
  const int num_pages = 100;

  for (int i = 0; i < num_pages; i++) {
    page_id_t page_id = disk_manager.AllocatePage();
    EXPECT_EQ(page_id, i + 1);
    allocated_pages.push_back(page_id);
  }

  EXPECT_EQ(allocated_pages.size(), num_pages);
}

// Test: Write and read a single page
TEST_F(DiskManagerTest, WriteAndReadSinglePage) {
  DiskManager disk_manager(test_db_file_);

  // Allocate a page
  page_id_t page_id = disk_manager.AllocatePage();

  // Create a page with some data
  char write_buffer[PAGE_SIZE];
  memset(write_buffer, 0, PAGE_SIZE);
  PageHeader* header = GetHeaderFromBuffer(write_buffer);
  header->page_id = page_id;
  header->slot_count = 0;
  header->free_start = sizeof(PageHeader);
  header->free_end = PAGE_SIZE;

  // Write some test data
  const char* test_data = "Hello, DiskManager!";
  memcpy(write_buffer + sizeof(PageHeader), test_data, strlen(test_data));

  // Write page to disk
  disk_manager.WritePage(page_id, write_buffer);

  // Read page back
  char read_buffer[PAGE_SIZE];
  memset(read_buffer, 0, PAGE_SIZE);
  disk_manager.ReadPage(page_id, read_buffer);

  // Verify data
  EXPECT_EQ(memcmp(write_buffer + sizeof(PageHeader),
                   read_buffer + sizeof(PageHeader), strlen(test_data)),
            0);

  PageHeader* read_header = GetHeaderFromBuffer(read_buffer);
  EXPECT_EQ(read_header->page_id, page_id);
}

// Test: Write and read multiple pages
TEST_F(DiskManagerTest, WriteAndReadMultiplePages) {
  DiskManager disk_manager(test_db_file_);

  const int num_pages = 10;
  std::vector<page_id_t> page_ids;

  // Allocate and write pages
  for (int i = 0; i < num_pages; i++) {
    page_id_t page_id = disk_manager.AllocatePage();
    page_ids.push_back(page_id);

    char write_buffer[PAGE_SIZE];
    memset(write_buffer, 0, PAGE_SIZE);
    PageHeader* header = GetHeaderFromBuffer(write_buffer);
    header->page_id = page_id;
    header->slot_count = i;
    header->free_start = sizeof(PageHeader);
    header->free_end = PAGE_SIZE;

    disk_manager.WritePage(page_id, write_buffer);
  }

  // Read pages back and verify
  for (int i = 0; i < num_pages; i++) {
    char read_buffer[PAGE_SIZE];
    disk_manager.ReadPage(page_ids[i], read_buffer);

    PageHeader* header = GetHeaderFromBuffer(read_buffer);
    EXPECT_EQ(header->page_id, page_ids[i]);
    EXPECT_EQ(header->slot_count, i);
  }
}

// Test: Persistence - data survives close and reopen
TEST_F(DiskManagerTest, DataPersistence) {
  page_id_t page_id;
  const char* test_data = "Persistent data test";

  // Write data
  {
    DiskManager disk_manager(test_db_file_);
    page_id = disk_manager.AllocatePage();

    char write_buffer[PAGE_SIZE];
    memset(write_buffer, 0, PAGE_SIZE);
    PageHeader* header = GetHeaderFromBuffer(write_buffer);
    header->page_id = page_id;
    header->slot_count = 0;
    header->free_start = sizeof(PageHeader);
    header->free_end = PAGE_SIZE;

    memcpy(write_buffer + sizeof(PageHeader), test_data, strlen(test_data));
    disk_manager.WritePage(page_id, write_buffer);
  }

  // Read data after reopen
  {
    DiskManager disk_manager(test_db_file_);
    char read_buffer[PAGE_SIZE];
    disk_manager.ReadPage(page_id, read_buffer);

    // Verify data
    EXPECT_EQ(
        memcmp(read_buffer + sizeof(PageHeader), test_data, strlen(test_data)),
        0);
  }
}

// Test: Deallocate page (currently a no-op but should not crash)
TEST_F(DiskManagerTest, DeallocatePage) {
  DiskManager disk_manager(test_db_file_);

  page_id_t page_id = disk_manager.AllocatePage();
  EXPECT_NO_THROW(disk_manager.DeallocatePage(page_id));
}

// Test: Write nullptr should throw
TEST_F(DiskManagerTest, WriteNullptrThrows) {
  DiskManager disk_manager(test_db_file_);
  page_id_t page_id = disk_manager.AllocatePage();

  EXPECT_THROW(disk_manager.WritePage(page_id, nullptr), std::invalid_argument);
}

// Test: Read nullptr should throw
TEST_F(DiskManagerTest, ReadNullptrThrows) {
  DiskManager disk_manager(test_db_file_);
  page_id_t page_id = disk_manager.AllocatePage();

  // Write a valid page first
  char write_buffer[PAGE_SIZE];
  memset(write_buffer, 0, PAGE_SIZE);
  PageHeader* header = GetHeaderFromBuffer(write_buffer);
  header->page_id = page_id;
  header->slot_count = 0;
  header->free_start = sizeof(PageHeader);
  header->free_end = PAGE_SIZE;
  disk_manager.WritePage(page_id, write_buffer);

  EXPECT_THROW(disk_manager.ReadPage(page_id, nullptr), std::invalid_argument);
}

// Test: Checksum verification on read
TEST_F(DiskManagerTest, ChecksumVerification) {
  DiskManager disk_manager(test_db_file_);
  page_id_t page_id = disk_manager.AllocatePage();

  // Write a page with valid checksum
  char write_buffer[PAGE_SIZE];
  memset(write_buffer, 0, PAGE_SIZE);
  PageHeader* header = GetHeaderFromBuffer(write_buffer);
  header->page_id = page_id;
  header->slot_count = 5;
  header->free_start = sizeof(PageHeader);
  header->free_end = PAGE_SIZE;

  disk_manager.WritePage(page_id, write_buffer);

  // Read should succeed with valid checksum
  char read_buffer[PAGE_SIZE];
  EXPECT_NO_THROW(disk_manager.ReadPage(page_id, read_buffer));
}

// Test: Overwrite existing page
TEST_F(DiskManagerTest, OverwritePage) {
  DiskManager disk_manager(test_db_file_);
  page_id_t page_id = disk_manager.AllocatePage();

  // Write first version
  char write_buffer1[PAGE_SIZE];
  memset(write_buffer1, 0, PAGE_SIZE);
  PageHeader* header1 = GetHeaderFromBuffer(write_buffer1);
  header1->page_id = page_id;
  header1->slot_count = 1;
  header1->free_start = sizeof(PageHeader);
  header1->free_end = PAGE_SIZE;
  disk_manager.WritePage(page_id, write_buffer1);

  // Write second version
  char write_buffer2[PAGE_SIZE];
  memset(write_buffer2, 0, PAGE_SIZE);
  PageHeader* header2 = GetHeaderFromBuffer(write_buffer2);
  header2->page_id = page_id;
  header2->slot_count = 10;
  header2->free_start = sizeof(PageHeader);
  header2->free_end = PAGE_SIZE;
  disk_manager.WritePage(page_id, write_buffer2);

  // Read and verify we get the second version
  char read_buffer[PAGE_SIZE];
  disk_manager.ReadPage(page_id, read_buffer);
  PageHeader* read_header = GetHeaderFromBuffer(read_buffer);
  EXPECT_EQ(read_header->slot_count, 10);
}

// Test: Concurrent page allocation
TEST_F(DiskManagerTest, ConcurrentPageAllocation) {
  DiskManager disk_manager(test_db_file_);

  const int num_threads = 4;
  const int pages_per_thread = 25;
  std::vector<std::thread> threads;
  std::vector<std::vector<page_id_t>> allocated_pages(num_threads);

  // Launch threads to allocate pages concurrently
  for (int i = 0; i < num_threads; i++) {
    threads.emplace_back([&, i]() {
      for (int j = 0; j < pages_per_thread; j++) {
        page_id_t page_id = disk_manager.AllocatePage();
        allocated_pages[i].push_back(page_id);
      }
    });
  }

  // Wait for all threads
  for (auto& thread : threads) {
    thread.join();
  }

  // Verify all page IDs are unique
  std::set<page_id_t> unique_pages;
  for (const auto& thread_pages : allocated_pages) {
    for (page_id_t page_id : thread_pages) {
      EXPECT_TRUE(unique_pages.insert(page_id).second)
          << "Duplicate page ID: " << page_id;
    }
  }

  EXPECT_EQ(unique_pages.size(), num_threads * pages_per_thread);
}

// Test: Concurrent reads (after writing)
TEST_F(DiskManagerTest, ConcurrentReads) {
  DiskManager disk_manager(test_db_file_);

  // Write a page first
  page_id_t page_id = disk_manager.AllocatePage();
  char write_buffer[PAGE_SIZE];
  memset(write_buffer, 0, PAGE_SIZE);
  PageHeader* header = GetHeaderFromBuffer(write_buffer);
  header->page_id = page_id;
  header->slot_count = 42;
  header->free_start = sizeof(PageHeader);
  header->free_end = PAGE_SIZE;
  disk_manager.WritePage(page_id, write_buffer);

  // Read concurrently from multiple threads
  const int num_threads = 8;
  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};

  for (int i = 0; i < num_threads; i++) {
    threads.emplace_back([&]() {
      char read_buffer[PAGE_SIZE];
      disk_manager.ReadPage(page_id, read_buffer);
      PageHeader* read_header = GetHeaderFromBuffer(read_buffer);
      if (read_header->slot_count == 42) {
        success_count++;
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_EQ(success_count, num_threads);
}

// Test: Large data write and read
TEST_F(DiskManagerTest, LargeDataWriteRead) {
  DiskManager disk_manager(test_db_file_);
  page_id_t page_id = disk_manager.AllocatePage();

  // Fill page with pattern
  char write_buffer[PAGE_SIZE];
  memset(write_buffer, 0, PAGE_SIZE);
  PageHeader* header = GetHeaderFromBuffer(write_buffer);
  header->page_id = page_id;
  header->slot_count = 0;
  header->free_start = sizeof(PageHeader);
  header->free_end = PAGE_SIZE;

  // Fill remaining space with pattern
  for (size_t i = sizeof(PageHeader); i < PAGE_SIZE; i++) {
    write_buffer[i] = static_cast<char>(i % 256);
  }

  disk_manager.WritePage(page_id, write_buffer);

  // Read back and verify
  char read_buffer[PAGE_SIZE];
  disk_manager.ReadPage(page_id, read_buffer);

  for (size_t i = sizeof(PageHeader); i < PAGE_SIZE; i++) {
    EXPECT_EQ(read_buffer[i], static_cast<char>(i % 256))
        << "Mismatch at byte " << i;
  }
}

// Test: Multiple database files simultaneously
TEST_F(DiskManagerTest, MultipleDatabaseFiles) {
  std::string test_db_file2_ =
      "/tmp/test/test_db2_" + std::to_string(GetTestId()) + ".db";

  {
    DiskManager disk_manager1(test_db_file_);
    DiskManager disk_manager2(test_db_file2_);

    EXPECT_TRUE(disk_manager1.IsOpen());
    EXPECT_TRUE(disk_manager2.IsOpen());

    page_id_t page_id1 = disk_manager1.AllocatePage();
    page_id_t page_id2 = disk_manager2.AllocatePage();

    char buffer1[PAGE_SIZE];
    memset(buffer1, 1, PAGE_SIZE);
    PageHeader* header1 = GetHeaderFromBuffer(buffer1);
    header1->page_id = page_id1;
    header1->slot_count = 10;
    header1->free_start = sizeof(PageHeader);
    header1->free_end = PAGE_SIZE;

    char buffer2[PAGE_SIZE];
    memset(buffer2, 2, PAGE_SIZE);
    PageHeader* header2 = GetHeaderFromBuffer(buffer2);
    header2->page_id = page_id2;
    header2->slot_count = 20;
    header2->free_start = sizeof(PageHeader);
    header2->free_end = PAGE_SIZE;

    disk_manager1.WritePage(page_id1, buffer1);
    disk_manager2.WritePage(page_id2, buffer2);

    char read_buffer1[PAGE_SIZE];
    char read_buffer2[PAGE_SIZE];

    disk_manager1.ReadPage(page_id1, read_buffer1);
    disk_manager2.ReadPage(page_id2, read_buffer2);

    PageHeader* read_header1 = GetHeaderFromBuffer(read_buffer1);
    PageHeader* read_header2 = GetHeaderFromBuffer(read_buffer2);

    EXPECT_EQ(read_header1->slot_count, 10);
    EXPECT_EQ(read_header2->slot_count, 20);
  }

  // Cleanup second file
  if (fs::exists(test_db_file2_)) {
    fs::remove(test_db_file2_);
  }
}

// Test: Next page ID persistence
TEST_F(DiskManagerTest, NextPageIdPersistence) {
  // Allocate some pages
  {
    DiskManager disk_manager(test_db_file_);
    disk_manager.AllocatePage();  // 1
    disk_manager.AllocatePage();  // 2
    disk_manager.AllocatePage();  // 3
  }

  // Reopen and continue allocating
  {
    DiskManager disk_manager(test_db_file_);
    page_id_t page_id = disk_manager.AllocatePage();
    EXPECT_EQ(page_id, 4);  // Should continue from where we left off
  }
}
