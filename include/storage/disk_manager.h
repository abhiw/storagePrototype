#ifndef STORAGEENGINE_DISK_MANAGER_H
#define STORAGEENGINE_DISK_MANAGER_H

// DiskManager provides low-level file I/O operations for database pages.
// It abstracts OS-level system calls, handles file layout, ensures data
// integrity through checksums, and provides thread-safe page access.

// Open/close database files
// Read/write 8KB pages at specific page IDs
// Manage file header with table metadata
// Allocate new pages
// Verify data integrity with checksums
// Thread-safe concurrent reads
// Error handling and retry logic

#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

#include "../common/types.h"

class DiskManager {
 public:
  DiskManager(const std::string& db_file_name);
  ~DiskManager();

  void ReadPage(page_id_t page_id, char* page_data) const;
  void WritePage(page_id_t page_id, const char* page_data) const;
  page_id_t AllocatePage();
  void DeallocatePage(page_id_t page_id);
  bool IsOpen();

  enum ErrorCode { ERROR_ALREADY_OPEN, ERROR_INVALID_FILENAME };

 private:
  std::string db_file_name_;
  int db_file_descriptor_;
  page_id_t next_page_id_;
  std::mutex metadata_mutex_;  // Only for metadata operations (not I/O)
  std::atomic<bool> is_open_;

  ErrorCode OpenDBFile();
  void CloseDBFile();
  uint32_t ComputeChecksum(const char* data, size_t length);

  struct FileHeader {
    char magic_number[4];     // "STOR"
    uint32_t version;         // File format version
    page_id_t next_page_id;   // Next available page ID
    uint32_t reserved[125];   // Padding to make header 512 bytes
    uint32_t table_id_;       // Unique table identifier
    uint32_t page_size_;      // Size of each page in bytes always 8192
    uint32_t page_count_;     // Total number of pages in the file
    char table_name_[64];     // Name of the table null terminated
    uint32_t schema_length_;  // Length of the schema definition
    uint32_t schema_offset_;  // Offset to the schema definition
  } file_header_;
};

#endif  // STORAGEENGINE_DISK_MANAGER_H
