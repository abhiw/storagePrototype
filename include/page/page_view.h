#ifndef STORAGEENGINE_PAGE_VIEW_H
#define STORAGEENGINE_PAGE_VIEW_H

#include "page.h"

// Non-owning view of a Page buffer
// IMPORTANT: PageView does NOT own the memory it points to!
// The caller must ensure the buffer outlives the PageView instance.
//
// Use this when you have a stack-allocated or externally-managed buffer
// and need to read/write page data without transferring ownership.
//
// Example (DiskManager use case):
//   char buffer[PAGE_SIZE];
//   PageView view(buffer);  // View doesn't own buffer
//   view.SetPageId(42);
//   view.ComputeChecksum();
//   // buffer still valid after view is destroyed
class PageView {
 public:
  // Construct a non-owning view over an external buffer
  // WARNING: Caller must ensure buffer lifetime exceeds PageView lifetime
  explicit PageView(char* buffer) : page_buffer_(buffer) {}

  // No destructor needed - doesn't own memory
  ~PageView() = default;

  // Delete copy/move - views should be short-lived and non-transferable
  // This prevents accidental misuse like storing views in containers
  PageView(const PageView&) = delete;
  PageView& operator=(const PageView&) = delete;
  PageView(PageView&&) = delete;
  PageView& operator=(PageView&&) = delete;

  // Same interface as Page class for reading/writing page data
  char* GetRawBuffer() const { return page_buffer_; }
  uint32_t ComputeChecksum() const;
  bool VerifyChecksum() const;

  // Getters
  uint16_t GetPageId() const;
  uint16_t GetSlotId() const;
  uint16_t GetFreeStart() const;
  uint16_t GetFreeEnd() const;
  uint16_t GetSlotCount() const;
  uint8_t GetPageType() const;
  uint8_t GetFlags() const;
  uint32_t GetChecksum() const;
  uint16_t GetDeletedSlotCount() const;
  bool IsDirty() const;
  size_t GetFragmentedBytes() const;

  // Setters
  void SetPageId(uint16_t page_id) const;
  void SetSlotId(uint16_t slot_id) const;
  void SetFreeStart(uint16_t free_start) const;
  void SetFreeEnd(uint16_t free_end) const;
  void SetSlotCount(uint16_t slot_count) const;
  void SetPageType(uint8_t page_type) const;
  void SetFlags(uint8_t flags) const;
  void SetChecksum(uint32_t checksum) const;
  void SetDeletedSlotCount(uint16_t deletedSlotCount);

 private:
  char* page_buffer_;  // Non-owning pointer - does NOT manage memory

  // Helper to get header from buffer
  PageHeader* GetHeader() const {
    return reinterpret_cast<PageHeader*>(page_buffer_);
  }
};

#endif  // STORAGEENGINE_PAGE_VIEW_H
