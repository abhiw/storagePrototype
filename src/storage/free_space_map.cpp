#include "../../include/storage/free_space_map.h"

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <iostream>

// Constructor
FreeSpaceMap::FreeSpaceMap(const std::string& fsm_file_name)
    : fsm_file_name_(fsm_file_name),
      fsm_fd_(-1),
      page_count_(0),
      is_dirty_(false),
      is_initialized_(false) {}

// Destructor
FreeSpaceMap::~FreeSpaceMap() {
  if (is_initialized_ && is_dirty_) {
    Flush();
  }
  CloseFSMFile();
}

// Initialize the FSM
bool FreeSpaceMap::Initialize() {
  std::lock_guard<std::mutex> lock(fsm_mutex_);

  if (is_initialized_) {
    return true;
  }

  // Open or create the FSM file
  if (!OpenFSMFile()) {
    return false;
  }

  // Try to load existing FSM data
  if (!LoadFromDisk()) {
    // If load fails, it might be a new file - initialize with empty FSM
    page_count_ = 0;
    fsm_cache_.clear();
    allocated_pages_.clear();
    is_dirty_ = true;
  }

  is_initialized_ = true;
  return true;
}

// Update page free space
void FreeSpaceMap::UpdatePageFreeSpace(page_id_t page_id,
                                       uint16_t available_bytes) {
  std::lock_guard<std::mutex> lock(fsm_mutex_);

  // Ensure cache has space for this page
  EnsureCapacity(page_id);

  uint8_t category = BytesToCategory(available_bytes);

  if (page_id < fsm_cache_.size()) {
    fsm_cache_[page_id] = category;
    // Track this page as allocated
    allocated_pages_.insert(page_id);
    is_dirty_ = true;

    if (page_id >= page_count_) {
      page_count_ = page_id + 1;
    }
  }
}

// Find a page with sufficient space
page_id_t FreeSpaceMap::FindPageWithSpace(uint16_t required_bytes) {
  std::lock_guard<std::mutex> lock(fsm_mutex_);

  // Convert required bytes to minimum category
  uint8_t min_category = BytesToCategory(required_bytes);

  // Scan ONLY allocated pages (handles sparse allocation)
  for (page_id_t page_id : allocated_pages_) {
    if (page_id < fsm_cache_.size()) {
      uint8_t page_category = fsm_cache_[page_id];

      if (page_category > min_category) {
        return page_id;
      }

      if (page_category == min_category && page_category > 0) {
        return page_id;
      }
    }
  }

  // No suitable page found - caller should allocate a new page
  return INVALID_PAGE_ID;
}

// Get category for a specific page
uint8_t FreeSpaceMap::GetCategory(page_id_t page_id) const {
  std::lock_guard<std::mutex> lock(fsm_mutex_);

  // Check if page is actually allocated
  if (allocated_pages_.find(page_id) == allocated_pages_.end()) {
    return 0;  // Page not allocated
  }

  if (page_id < fsm_cache_.size()) {
    return fsm_cache_[page_id];
  }
  return 0;  // Page not tracked or no free space
}

// Set category for a specific page
void FreeSpaceMap::SetCategory(page_id_t page_id, uint8_t category) {
  std::lock_guard<std::mutex> lock(fsm_mutex_);

  EnsureCapacity(page_id);

  if (page_id < fsm_cache_.size()) {
    fsm_cache_[page_id] = category;
    allocated_pages_.insert(page_id);  // Mark as allocated
    is_dirty_ = true;

    if (page_id >= page_count_) {
      page_count_ = page_id + 1;
    }
  }
}

// Get total page count
page_id_t FreeSpaceMap::GetPageCount() const {
  std::lock_guard<std::mutex> lock(fsm_mutex_);
  return page_count_;
}

// Flush to disk
bool FreeSpaceMap::Flush() {
  std::lock_guard<std::mutex> lock(fsm_mutex_);

  if (!is_dirty_) {
    return true;  // Nothing to flush
  }

  bool success = WriteToDisk();
  if (success) {
    is_dirty_ = false;
  }
  return success;
}

// Convert bytes to category
uint8_t FreeSpaceMap::BytesToCategory(uint16_t available_bytes) {
  // Clamp to PAGE_SIZE
  if (available_bytes > PAGE_SIZE) {
    available_bytes = PAGE_SIZE;
  }

  // Formula: (available_bytes * 255) / 8192
  // Use 64-bit intermediate to avoid overflow
  uint64_t category =
      (static_cast<uint64_t>(available_bytes) * MAX_CATEGORY) / PAGE_SIZE;
  return static_cast<uint8_t>(category);
}

// Convert category to bytes
uint16_t FreeSpaceMap::CategoryToBytes(uint8_t category) {
  // Reverse formula: (category * 8192) / 255
  // Use 64-bit intermediate to avoid overflow
  uint64_t bytes = (static_cast<uint64_t>(category) * PAGE_SIZE) / MAX_CATEGORY;
  return static_cast<uint16_t>(bytes);
}

// Open FSM file
bool FreeSpaceMap::OpenFSMFile() {
  // Open file with read/write, create if doesn't exist
  fsm_fd_ = open(fsm_file_name_.c_str(), O_RDWR | O_CREAT, 0644);

  if (fsm_fd_ < 0) {
    std::cerr << "Failed to open FSM file: " << fsm_file_name_ << std::endl;
    return false;
  }

  return true;
}

// Close FSM file
void FreeSpaceMap::CloseFSMFile() {
  if (fsm_fd_ >= 0) {
    close(fsm_fd_);
    fsm_fd_ = -1;
  }
}

// Load from disk using pread()
bool FreeSpaceMap::LoadFromDisk() {
  if (fsm_fd_ < 0) {
    return false;
  }

  // Get file size
  off_t file_size = lseek(fsm_fd_, 0, SEEK_END);
  if (file_size < 0) {
    return false;
  }

  // If file is empty or too small, it's a new FSM
  if (file_size < static_cast<off_t>(FSM_HEADER_SIZE + sizeof(uint32_t))) {
    return false;  // Need at least magic + page_count + allocated_count
  }

  // Read header using pread() - thread-safe, no lseek needed
  off_t offset = 0;

  // Read magic number
  uint32_t magic = 0;
  if (pread(fsm_fd_, &magic, sizeof(magic), offset) != sizeof(magic)) {
    return false;
  }
  offset += sizeof(magic);

  // Verify magic number
  if (magic != FSM_MAGIC_NUMBER) {
    std::cerr << "Invalid FSM magic number: " << std::hex << magic << std::endl;
    return false;
  }

  // Read page count
  uint32_t stored_page_count = 0;
  if (pread(fsm_fd_, &stored_page_count, sizeof(stored_page_count), offset) !=
      sizeof(stored_page_count)) {
    return false;
  }
  offset += sizeof(stored_page_count);
  page_count_ = stored_page_count;

  // Read allocated pages count
  uint32_t allocated_count = 0;
  if (pread(fsm_fd_, &allocated_count, sizeof(allocated_count), offset) !=
      sizeof(allocated_count)) {
    return false;
  }
  offset += sizeof(allocated_count);

  if (allocated_count == 0) {
    return true;  // Empty FSM is valid
  }

  // Read allocated page IDs
  std::vector<page_id_t> allocated_page_ids(allocated_count);
  size_t ids_size = allocated_count * sizeof(page_id_t);
  if (pread(fsm_fd_, allocated_page_ids.data(), ids_size, offset) !=
      static_cast<ssize_t>(ids_size)) {
    std::cerr << "Failed to read allocated page IDs" << std::endl;
    return false;
  }
  offset += ids_size;

  // Populate allocated_pages_ set
  allocated_pages_.clear();
  for (page_id_t page_id : allocated_page_ids) {
    allocated_pages_.insert(page_id);
  }

  // Read category data for all pages (dense array)
  if (page_count_ > 0) {
    fsm_cache_.resize(page_count_);
    if (pread(fsm_fd_, fsm_cache_.data(), page_count_, offset) !=
        static_cast<ssize_t>(page_count_)) {
      std::cerr << "Failed to read FSM categories" << std::endl;
      return false;
    }
  }

  is_dirty_ = false;
  return true;
}

// Write to disk using pwrite()
bool FreeSpaceMap::WriteToDisk() {
  if (fsm_fd_ < 0) {
    return false;
  }

  off_t offset = 0;

  // Write magic number using pwrite() - thread-safe
  uint32_t magic = FSM_MAGIC_NUMBER;
  if (pwrite(fsm_fd_, &magic, sizeof(magic), offset) != sizeof(magic)) {
    return false;
  }
  offset += sizeof(magic);

  // Write page count
  uint32_t stored_page_count = page_count_;
  if (pwrite(fsm_fd_, &stored_page_count, sizeof(stored_page_count), offset) !=
      sizeof(stored_page_count)) {
    return false;
  }
  offset += sizeof(stored_page_count);

  // Write allocated pages count
  uint32_t allocated_count = static_cast<uint32_t>(allocated_pages_.size());
  if (pwrite(fsm_fd_, &allocated_count, sizeof(allocated_count), offset) !=
      sizeof(allocated_count)) {
    return false;
  }
  offset += sizeof(allocated_count);

  // Write allocated page IDs
  if (allocated_count > 0) {
    std::vector<page_id_t> allocated_page_ids(allocated_pages_.begin(),
                                              allocated_pages_.end());
    size_t ids_size = allocated_count * sizeof(page_id_t);
    if (pwrite(fsm_fd_, allocated_page_ids.data(), ids_size, offset) !=
        static_cast<ssize_t>(ids_size)) {
      return false;
    }
    offset += ids_size;
  }

  // Write category data (dense array)
  if (page_count_ > 0 && !fsm_cache_.empty()) {
    size_t bytes_to_write =
        std::min(static_cast<size_t>(page_count_), fsm_cache_.size());
    if (pwrite(fsm_fd_, fsm_cache_.data(), bytes_to_write, offset) !=
        static_cast<ssize_t>(bytes_to_write)) {
      return false;
    }
    offset += bytes_to_write;
  }

  // Truncate file to exact size (remove any old data)
  if (ftruncate(fsm_fd_, offset) < 0) {
    std::cerr << "Failed to truncate FSM file" << std::endl;
    // Not fatal, continue
  }

  // Sync to disk
  if (fsync(fsm_fd_) < 0) {
    return false;
  }

  return true;
}

// Ensure cache capacity
void FreeSpaceMap::EnsureCapacity(page_id_t page_id) {
  // Expand cache if necessary
  if (page_id >= fsm_cache_.size()) {
    // Grow cache with some headroom to reduce reallocations
    size_t new_size =
        std::max(static_cast<size_t>(page_id + 1), fsm_cache_.size() * 2);

    // Limit growth to reasonable size (support hierarchical FSM later)
    if (new_size > FSM_CATEGORIES_PER_PAGE * 100) {
      new_size = page_id + 1;
    }

    fsm_cache_.resize(new_size, 0);  // Initialize new entries to 0 (no space)
  }
}

// Get FSM page index (for hierarchical FSM)
size_t FreeSpaceMap::GetFSMPageIndex(page_id_t page_id) const {
  return page_id / FSM_CATEGORIES_PER_PAGE;
}

// Get FSM page offset (for hierarchical FSM)
size_t FreeSpaceMap::GetFSMPageOffset(page_id_t page_id) const {
  return page_id % FSM_CATEGORIES_PER_PAGE;
}
