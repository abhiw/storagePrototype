#ifndef STORAGEENGINE_FREE_SPACE_MAP_H
#define STORAGEENGINE_FREE_SPACE_MAP_H

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "../common/config.h"
#include "../common/types.h"

// Free Space Map (FSM) tracks available space in data pages for efficient
// page allocation during tuple insertion. The FSM uses a category-based
// encoding where each page's free space is represented as a single byte
// value (0-255), allowing for compact storage and fast lookups.
//
// FSM Page Format (8192 bytes):
//   - Magic number: 0x46534D00 (4 bytes)
//   - Page count: number of data pages tracked (4 bytes)
//   - Category bytes: one byte per data page (8184 bytes)
//
// Category encoding: (available_bytes * 255) / 8192
//   - 0 = no free space
//   - 255 = completely empty page
//
// For tables with >8184 pages, the FSM uses a hierarchical structure
// where multiple FSM pages are linked together.

// void CreateTable(string table_name, Schema schema) {
//  string data_file = table_name + ".db";
//  string fsm_file = table_name + ".fsm";
//
//  disk_manager_ = new DiskManager(data_file);
//  page_manager_ = new PageManager(disk_manager_,
//                                   new FreeSpaceMap(fsm_file));
// }

class FreeSpaceMap {
 public:
  // Constructor: takes the FSM file name
  explicit FreeSpaceMap(const std::string& fsm_file_name);

  // Destructor: ensures data is persisted
  ~FreeSpaceMap();

  // Initialize the FSM file (create new or load existing)
  // Returns true on success, false on failure
  bool Initialize();

  // Update the free space category for a given page
  // page_id: the data page ID
  // available_bytes: number of free bytes in the page (0-8192)
  void UpdatePageFreeSpace(page_id_t page_id, uint16_t available_bytes);

  // Find a page with at least the required amount of free space
  // required_bytes: minimum free space needed
  // Returns: page_id of suitable page, or INVALID_PAGE_ID if none found
  page_id_t FindPageWithSpace(uint16_t required_bytes);

  // Get the category value for a specific page
  // page_id: the data page ID
  // Returns: category value (0-255)
  uint8_t GetCategory(page_id_t page_id) const;

  // Set the category value for a specific page
  // page_id: the data page ID
  // category: category value (0-255)
  void SetCategory(page_id_t page_id, uint8_t category);

  // Get the total number of pages tracked by this FSM
  page_id_t GetPageCount() const;

  // Flush the in-memory FSM cache to disk
  // Returns true on success, false on failure
  bool Flush();

  // Constants for FSM page format
  static constexpr uint32_t FSM_MAGIC_NUMBER = 0x46534D00;
  static constexpr size_t FSM_HEADER_SIZE = 8;             // magic + page_count
  static constexpr size_t FSM_CATEGORIES_PER_PAGE = 8184;  // 8192 - 8
  static constexpr size_t FSM_PAGE_SIZE = 8192;

  // Category encoding/decoding
  static constexpr uint8_t MAX_CATEGORY = 255;

  // Convert available bytes to category
  static uint8_t BytesToCategory(uint16_t available_bytes);

  // Convert category to approximate available bytes
  static uint16_t CategoryToBytes(uint8_t category);

 private:
  // FSM file name
  std::string fsm_file_name_;

  // File descriptor for the FSM file
  int fsm_fd_;

  // In-memory cache of FSM data (category bytes)
  // For large tables, this may contain multiple FSM pages worth of data
  // Uses dense array for O(1) access, but only allocated pages are valid
  std::vector<uint8_t> fsm_cache_;

  // Set of actually allocated page IDs (sparse representation)
  // This allows non-sequential page allocation (0, 5, 17, 100)
  // Only pages in this set are scanned by FindPageWithSpace()
  std::unordered_set<page_id_t> allocated_pages_;

  // Total number of data pages tracked (highest page_id + 1)
  // This is NOT the count of allocated pages, but the size of the dense array
  page_id_t page_count_;

  // Mutex for thread-safe access to FSM
  mutable std::mutex fsm_mutex_;

  // Flag indicating if FSM has been modified (dirty)
  bool is_dirty_;

  // Flag indicating if FSM is initialized
  bool is_initialized_;

  // Helper: Open the FSM file (create if doesn't exist)
  bool OpenFSMFile();

  // Helper: Close the FSM file
  void CloseFSMFile();

  // Helper: Load FSM data from disk into cache
  bool LoadFromDisk();

  // Helper: Write FSM cache to disk
  bool WriteToDisk();

  // Helper: Ensure the FSM cache is large enough for the given page_id
  void EnsureCapacity(page_id_t page_id);

  // Helper: Get the FSM page index for a given data page_id
  // (for hierarchical FSM support)
  size_t GetFSMPageIndex(page_id_t page_id) const;

  // Helper: Get the offset within an FSM page for a given data page_id
  size_t GetFSMPageOffset(page_id_t page_id) const;
};

#endif  // STORAGEENGINE_FREE_SPACE_MAP_H
