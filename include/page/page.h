//
// Created by Abhi on 01/12/25.
//

#ifndef STORAGEENGINE_PAGE_H
#define STORAGEENGINE_PAGE_H

#include <cstddef>
#include <cstdint>

#include "../common/config.h"
#include "../common/types.h"

constexpr size_t SLOT_ENTRY_SIZE = 8;

typedef struct PageHeader {
  uint16_t page_id;     // 2
  uint16_t slot_id;     // 2
  uint16_t free_start;  // 2
  uint16_t free_end;    // 2
  uint16_t slot_count;  // 2
  uint8_t page_type;    // 1
  uint8_t flags;        // 1
  uint32_t checksum;    // 4

  // Runtime metadata (NOT stored on disk)
  uint16_t deleted_tuple_count_;  // Cached count of deleted slots
  size_t fragmented_bytes_;       // Cached sum of deleted tuple sizes
  bool is_dirty_;                 // Has page been modified?
} PageHeader;

// Slot entry flags
constexpr uint8_t SLOT_VALID = 0x01;       // bit 0: slot is valid
constexpr uint8_t SLOT_FORWARDED = 0x02;   // bit 1: slot is forwarded
constexpr uint8_t SLOT_COMPRESSED = 0x04;  // bit 2: slot data is compressed

#pragma pack(push, 1)
typedef struct SlotEntry {
  uint16_t offset;      // 2 bytes: offset of data in page
  uint16_t length;      // 2 bytes: length of data
  uint8_t flags;        // 1 byte: status flags
  uint8_t next_ptr[3];  // 3 bytes: 24-bit forwarding pointer (16-bit page_id +
                        // 8-bit slot_id)
} SlotEntry;
#pragma pack(pop)

static_assert(sizeof(SlotEntry) == 8, "SlotEntry must be exactly 8 bytes");

class Page {
 public:
  // Constructor
  Page() : page_buffer_(nullptr) {}

  // Get raw buffer pointer
  char* GetRawBuffer() const { return page_buffer_.get(); }

  uint32_t ComputeChecksum() const;
  static std::unique_ptr<Page> CreateNew();
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
  bool IsDirty() const;
  uint16_t GetDeletedTupleCount() const;
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

  // Slot directory methods
  SlotEntry& GetSlotEntry(slot_id_t slot_id) const;
  slot_id_t AddSlot(uint16_t offset, uint16_t length) const;
  void MarkSlotDeleted(slot_id_t slot_id) const;
  bool IsSlotValid(slot_id_t slot_id) const;
  bool IsSlotForwarded(slot_id_t slot_id) const;
  TupleId GetForwardingPointer(slot_id_t slot_id) const;
  void SetForwardingPointer(slot_id_t slot_id, page_id_t page_id,
                            slot_id_t target_slot_id) const;
  slot_id_t InsertTuple(const char* tuple_data, uint16_t tuple_size) const;
  ErrorCode DeleteTuple(slot_id_t slot_id) const;
  void RecomputeFragmentationStats() const;
  bool ShouldCompact() const;
  void CompactPage() const;

  // Update operations
  ErrorCode UpdateTupleInPlace(slot_id_t slot_id, const char* new_data,
                               uint16_t new_size) const;
  ErrorCode MarkSlotForwarded(slot_id_t slot_id, page_id_t target_page_id,
                              slot_id_t target_slot_id) const;
  TupleId FollowForwardingChain(slot_id_t slot_id, int max_hops = 10) const;

 private:
  // RAII-managed pointer to the entire page buffer (8KB)
  // The buffer layout is:
  //   [PageHeader][Data Area][Slot Directory growing from end]
  // AlignedBuffer automatically frees aligned memory in destructor
  AlignedBuffer page_buffer_;

  // Helper to get header from buffer
  PageHeader* GetHeader() const;

  // Helper to get slot entry address
  SlotEntry* GetSlotEntryPtr(slot_id_t slot_id) const;

  size_t GetAvailableFreeSpace() const;
  slot_id_t FindDeletedSlot() const;
};

struct TupleInfo {
  slot_id_t original_slot_id;
  uint16_t offset;
  uint16_t length;
  uint8_t flags;
  uint8_t next_ptr[3];
};

#endif  // STORAGEENGINE_PAGE_H
