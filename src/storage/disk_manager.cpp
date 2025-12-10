#include "../../include/storage/disk_manager.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>

#include "../../include/common/checksum.h"
#include "../../include/common/logger.h"
#include "../../include/page/page.h"
#include "../../include/page/page_view.h"

//  - ReadPage() - NO LOCK (pread is thread-safe)
//  - WritePage() - NO LOCK (pwrite is thread-safe)
//  mutex for metadata operations only:
//   - OpenDBFile() / CloseDBFile() - File descriptor and state changes
//   - AllocatePage() - Updates next_page_id_ and page count
//   - DeallocatePage() - no-op for now
DiskManager::DiskManager(const std::string& db_file_name)
    : db_file_name_(db_file_name),
      db_file_descriptor_(-1),
      next_page_id_(0),
      is_open_(false) {
  LOG_INFO_STREAM("DiskManager: Initializing with file: " << db_file_name);
  ErrorCode errorCode = OpenDBFile();
  if (errorCode != ERROR_ALREADY_OPEN) {
    LOG_INFO_STREAM(
        "DiskManager: Successfully opened database file: " << db_file_name);
  } else {
    LOG_WARNING_STREAM(
        "DiskManager: Database file already open: " << db_file_name);
  }
}

DiskManager::~DiskManager() {
  LOG_INFO_STREAM(
      "DiskManager: Destroying disk manager for file: " << db_file_name_);
  CloseDBFile();
}

DiskManager::ErrorCode DiskManager::OpenDBFile() {
  std::lock_guard<std::mutex> lock(metadata_mutex_);

  if (is_open_) {
    LOG_WARNING_STREAM("DiskManager: File already open: " << db_file_name_);
    return ERROR_ALREADY_OPEN;
  }

  if (db_file_name_.empty()) {
    LOG_ERROR_STREAM("DiskManager: Invalid filename (empty string)");
    return ERROR_INVALID_FILENAME;
  }

  // Check if file exists
  struct stat buffer;
  bool file_exists = (stat(db_file_name_.c_str(), &buffer) == 0);

  // Open file with read/write permissions, create if doesn't exist
  db_file_descriptor_ =
      open(db_file_name_.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

  if (db_file_descriptor_ < 0) {
    LOG_ERROR_STREAM("DiskManager: Failed to open file: "
                     << db_file_name_ << " errno: " << errno);
    throw std::runtime_error("Failed to open database file: " + db_file_name_);
  }

  if (!file_exists) {
    // New file - initialize header
    LOG_INFO_STREAM(
        "DiskManager: Creating new database file: " << db_file_name_);

    memset(&file_header_, 0, sizeof(FileHeader));
    memcpy(file_header_.magic_number, "STOR", 4);
    file_header_.version = 1;
    file_header_.next_page_id = 1;  // Start from 1 (0 is INVALID_PAGE_ID)
    file_header_.page_size_ = PAGE_SIZE;
    file_header_.page_count_ = 0;
    next_page_id_ = 1;  // Initialize next_page_id_

    // Write header to file using pwrite()
    ssize_t bytes_written =
        pwrite(db_file_descriptor_, &file_header_, sizeof(FileHeader), 0);
    if (bytes_written != sizeof(FileHeader)) {
      LOG_ERROR_STREAM("DiskManager: Failed to write file header");
      close(db_file_descriptor_);
      db_file_descriptor_ = -1;
      throw std::runtime_error("Failed to write database file header");
    }

    // Sync to disk
    fsync(db_file_descriptor_);
  } else {
    // Existing file - read header using pread()
    LOG_INFO_STREAM(
        "DiskManager: Opening existing database file: " << db_file_name_);

    ssize_t bytes_read =
        pread(db_file_descriptor_, &file_header_, sizeof(FileHeader), 0);
    if (bytes_read != sizeof(FileHeader)) {
      LOG_ERROR_STREAM("DiskManager: Failed to read file header");
      close(db_file_descriptor_);
      db_file_descriptor_ = -1;
      throw std::runtime_error("Failed to read database file header");
    }

    // Verify magic number
    if (memcmp(file_header_.magic_number, "STOR", 4) != 0) {
      LOG_ERROR_STREAM("DiskManager: Invalid magic number in file header");
      close(db_file_descriptor_);
      db_file_descriptor_ = -1;
      throw std::runtime_error("Invalid database file format");
    }

    next_page_id_ = file_header_.next_page_id;
    LOG_INFO_STREAM(
        "DiskManager: Loaded existing file, next_page_id: " << next_page_id_);
  }

  is_open_ = true;
  return static_cast<ErrorCode>(0);  // Success
}

void DiskManager::CloseDBFile() {
  std::lock_guard<std::mutex> lock(metadata_mutex_);

  if (!is_open_ || db_file_descriptor_ < 0) {
    return;
  }

  // Update header with current next_page_id
  file_header_.next_page_id = next_page_id_;
  pwrite(db_file_descriptor_, &file_header_, sizeof(FileHeader), 0);

  // Sync and close
  fsync(db_file_descriptor_);
  close(db_file_descriptor_);

  db_file_descriptor_ = -1;
  is_open_ = false;

  LOG_INFO_STREAM("DiskManager: Closed database file: " << db_file_name_);
}

void DiskManager::ReadPage(page_id_t page_id, char* page_data) const {
  // No lock needed - pread() is thread-safe!

  if (!is_open_ || db_file_descriptor_ < 0) {
    LOG_ERROR_STREAM("DiskManager: Cannot read page, file not open");
    throw std::runtime_error("Database file not open");
  }

  if (page_data == nullptr) {
    LOG_ERROR_STREAM("DiskManager: Invalid page_data pointer (nullptr)");
    throw std::invalid_argument("page_data cannot be nullptr");
  }

  // Calculate offset: skip file header, then page_id * PAGE_SIZE
  off_t offset = sizeof(FileHeader) + (page_id * PAGE_SIZE);

  // pread() is thread-safe - atomically reads at offset without modifying fd
  // position
  ssize_t bytes_read = pread(db_file_descriptor_, page_data, PAGE_SIZE, offset);
  if (bytes_read != PAGE_SIZE) {
    LOG_ERROR_STREAM("DiskManager: Failed to read page "
                     << page_id << ", bytes_read: " << bytes_read);
    throw std::runtime_error("Failed to read page from disk");
  }

  // Initialize runtime metadata after reading from disk
  auto* page_header = reinterpret_cast<PageHeader*>(page_data);
  page_header->deleted_tuple_count_ = 0;
  page_header->fragmented_bytes_ = 0;
  page_header->is_dirty_ = false;  // Just loaded from disk, not modified yet

  // Recompute fragmentation stats by scanning slot directory
  uint16_t slot_count = page_header->slot_count;
  for (slot_id_t i = 0; i < slot_count; i++) {
    size_t slot_offset = PAGE_SIZE - (i + 1) * SLOT_ENTRY_SIZE;
    auto* slot_entry = reinterpret_cast<SlotEntry*>(
        reinterpret_cast<uint8_t*>(page_data) + slot_offset);

    if (!(slot_entry->flags & SLOT_VALID)) {
      page_header->deleted_tuple_count_++;
      page_header->fragmented_bytes_ += slot_entry->length;
    }
  }

  // Verify checksum using PageView (non-owning access to external buffer)
  PageView page_view(page_data);
  if (!page_view.VerifyChecksum()) {
    LOG_ERROR_STREAM("DiskManager: Checksum verification failed for page "
                     << page_id);
    throw std::runtime_error("Page checksum verification failed");
  }

  LOG_INFO_STREAM("DiskManager: Successfully read page " << page_id);
}

void DiskManager::WritePage(page_id_t page_id, const char* page_data) const {
  if (!is_open_ || db_file_descriptor_ < 0) {
    LOG_ERROR_STREAM("DiskManager: Cannot write page, file not open");
    throw std::runtime_error("Database file not open");
  }

  if (page_data == nullptr) {
    LOG_ERROR_STREAM("DiskManager: Invalid page_data pointer (nullptr)");
    throw std::invalid_argument("page_data cannot be nullptr");
  }

  // The PageHeader contains runtime only fields that should not be persisted:
  //   - deleted_tuple_count_ (bytes 16-17)
  //   - fragmented_bytes_ (bytes 24-31)
  //   - is_dirty_ (byte  32)
  PageHeader* page_header =
      reinterpret_cast<PageHeader*>(const_cast<char*>(page_data));
  page_header->deleted_tuple_count_ = 0;
  page_header->fragmented_bytes_ = 0;
  page_header->is_dirty_ = false;

  // Update checksum before writing
  // We need to cast away const to update the checksum in the buffer
  char* mutable_page_data = const_cast<char*>(page_data);
  PageView page_view(mutable_page_data);
  uint32_t checksum = page_view.ComputeChecksum();
  page_view.SetChecksum(checksum);

  // Calculate offset: skip file header, then page_id * PAGE_SIZE
  off_t offset = sizeof(FileHeader) + (page_id * PAGE_SIZE);

  // pwrite() is thread-safe  atomically writes at offset without modifying fd
  // position
  ssize_t bytes_written =
      pwrite(db_file_descriptor_, page_data, PAGE_SIZE, offset);
  if (bytes_written != PAGE_SIZE) {
    LOG_ERROR_STREAM("DiskManager: Failed to write page "
                     << page_id << ", bytes_written: " << bytes_written);
    throw std::runtime_error("Failed to write page to disk");
  }

  fsync(db_file_descriptor_);

  LOG_INFO_STREAM("DiskManager: Successfully wrote page " << page_id);
}

page_id_t DiskManager::AllocatePage() {
  std::lock_guard<std::mutex> lock(metadata_mutex_);

  if (!is_open_) {
    LOG_ERROR_STREAM("DiskManager: Cannot allocate page, file not open");
    throw std::runtime_error("Database file not open");
  }

  page_id_t new_page_id = next_page_id_++;
  file_header_.page_count_++;

  LOG_INFO_STREAM("DiskManager: Allocated new page " << new_page_id);

  return new_page_id;
}

void DiskManager::DeallocatePage(page_id_t page_id) {
  std::lock_guard<std::mutex> lock(metadata_mutex_);

  if (!is_open_) {
    LOG_ERROR_STREAM("DiskManager: Cannot deallocate page, file not open");
    throw std::runtime_error("Database file not open");
  }

  // This is a no-op. In a real implementation, we would:
  // - Add page_id to a free page list
  // - Update file header to track free pages
  // - Reuse deallocated pages in AllocatePage()

  LOG_INFO_STREAM("DiskManager: Deallocated page " << page_id
                                                   << " (no-op for now)");
}

bool DiskManager::IsOpen() {
  // No lock needed - is_open_ is atomic
  return is_open_;
}

uint32_t DiskManager::ComputeChecksum(const char* data, size_t length) {
  return checksum::Compute(reinterpret_cast<const uint8_t*>(data), length);
}