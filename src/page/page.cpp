//
// Created by Abhi on 01/12/25.
//

#include "../include/page/page.h"

#include <common/config.h>

#include <cstdlib>
#include <cstring>

#include "../include/common/checksum.h"
#include "../include/common/logger.h"

PageHeader* Page::GetHeader() const {
  return reinterpret_cast<PageHeader*>(page_buffer_.get());
}

uint32_t Page::ComputeChecksum() const {
  if (page_buffer_.get() == nullptr) {
    return 0;
  }

  // ========================================================================
  // Checksum Computation Strategy
  // ========================================================================
  // PageHeader layout (total size: 40 bytes):
  //   Bytes  0-11: On-disk persistent fields (page_id, slot_id, free_start,
  //   etc.) Bytes 12-15: checksum field (excluded from checksum calculation)
  //   Bytes 16-39: Runtime metadata (deleted_tuple_count_, fragmented_bytes_,
  //   is_dirty_) Bytes 40-8191: Page data (tuple data, free space, slot
  //   directory)
  //
  // Checksum coverage:
  //   ✓ Part 1: Bytes  0-11   (on-disk persistent header fields)
  //   ✗ Part 2: Bytes 12-15   (checksum field itself - treated as zeros)
  //   ✗ Part 3: Bytes 16-39   (EXCLUDED - runtime fields not persisted)
  //   ✓ Part 4: Bytes 40-8191 (page data area)
  //
  // Rationale:
  //   Runtime fields (deleted_tuple_count_, fragmented_bytes_, is_dirty_)
  //   are computed at runtime and should NOT affect the checksum. They are
  //   zeroed before WritePage() and recomputed after ReadPage().
  // ========================================================================

  const auto* page_data = reinterpret_cast<const uint8_t*>(page_buffer_.get());
  const size_t checksum_offset = offsetof(PageHeader, checksum);
  const size_t checksum_size = sizeof(PageHeader::checksum);

  // Initialize CRC
  uint32_t result_crc = checksum::Init();

  // Part 1: Checksum persistent header fields (bytes 0-11)
  result_crc = checksum::Update(result_crc, page_data, checksum_offset);

  // Part 2: Skip checksum field (bytes 12-15) - treat as zeros to avoid
  // circular dependency
  const uint32_t zero_checksum = 0;
  result_crc = checksum::Update(
      result_crc, reinterpret_cast<const uint8_t*>(&zero_checksum),
      checksum_size);

  // Part 3: Skip runtime metadata fields (bytes 16-39)
  // Part 4: Checksum page data area (bytes 40-8191)
  const size_t remaining_offset =
      sizeof(PageHeader);  // Start after entire PageHeader (40 bytes)
  const size_t remaining_size =
      PAGE_SIZE - remaining_offset;  // 8192 - 40 = 8152 bytes
  result_crc = checksum::Update(result_crc, page_data + remaining_offset,
                                remaining_size);

  // Finalize CRC
  return checksum::Finalize(result_crc);
}

std::unique_ptr<Page> Page::CreateNew() {
  // Allocate raw page data (8KB) with proper alignment
  void* raw_page_data = std::aligned_alloc(PAGE_SIZE, PAGE_SIZE);
  if (raw_page_data == nullptr) {
    LOG_ERROR("Page::CreateNew: Failed to allocate page memory");
    return nullptr;
  }

  // Zero out the page data
  std::memset(raw_page_data, 0, PAGE_SIZE);

  // Create Page object with unique_ptr
  auto new_page = std::make_unique<Page>();

  // Transfer ownership of aligned buffer to Page using AlignedBuffer (RAII)
  // AlignedDeleter will call std::free() when the buffer is destroyed
  new_page->page_buffer_ =
      AlignedBuffer(reinterpret_cast<char*>(raw_page_data), AlignedDeleter());

  // Initialize header fields
  PageHeader* header = new_page->GetHeader();
  header->page_id = 0;
  header->slot_id = 0;
  header->free_start = sizeof(PageHeader);
  header->free_end = PAGE_SIZE;
  header->slot_count = 0;
  header->page_type = 0;
  header->flags = 0;
  header->checksum = 0;

  // Initialize runtime metadata (not stored on disk)
  header->deleted_tuple_count_ = 0;
  header->fragmented_bytes_ = 0;
  header->is_dirty_ = true;  // New page is dirty until written to disk

  header->checksum = new_page->ComputeChecksum();

  LOG_INFO_STREAM("Page::CreateNew: Created new page with ID "
                  << header->page_id);
  return new_page;
}

bool Page::VerifyChecksum() const {
  if (page_buffer_.get() == nullptr) {
    LOG_ERROR("Page::VerifyChecksum: Page buffer is null");
    return false;
  }

  PageHeader* header = GetHeader();
  uint32_t computed = ComputeChecksum();
  bool valid = (header->checksum == computed);

  if (!valid) {
    LOG_ERROR_STREAM("Page::VerifyChecksum: Checksum mismatch for page "
                     << header->page_id << " (expected: " << header->checksum
                     << ", computed: " << computed << ")");
  }

  return valid;
}

// Getters
uint16_t Page::GetPageId() const {
  return page_buffer_.get() ? GetHeader()->page_id : 0;
}

uint16_t Page::GetSlotId() const {
  return page_buffer_.get() ? GetHeader()->slot_id : 0;
}

uint16_t Page::GetFreeStart() const {
  return page_buffer_.get() ? GetHeader()->free_start : 0;
}

uint16_t Page::GetFreeEnd() const {
  return page_buffer_.get() ? GetHeader()->free_end : 0;
}

uint16_t Page::GetSlotCount() const {
  return page_buffer_.get() ? GetHeader()->slot_count : 0;
}

uint8_t Page::GetPageType() const {
  return page_buffer_.get() ? GetHeader()->page_type : 0;
}

uint8_t Page::GetFlags() const {
  return page_buffer_.get() ? GetHeader()->flags : 0;
}

uint32_t Page::GetChecksum() const {
  return page_buffer_.get() ? GetHeader()->checksum : 0;
}

// Setters
void Page::SetPageId(uint16_t page_id) const {
  if (page_buffer_.get()) {
    GetHeader()->page_id = page_id;
  }
}

void Page::SetSlotId(uint16_t slot_id) const {
  if (page_buffer_.get()) {
    GetHeader()->slot_id = slot_id;
  }
}

void Page::SetFreeStart(const uint16_t free_start) const {
  if (page_buffer_.get()) {
    GetHeader()->free_start = free_start;
  }
}

void Page::SetFreeEnd(const uint16_t free_end) const {
  if (page_buffer_.get()) {
    GetHeader()->free_end = free_end;
  }
}

void Page::SetSlotCount(const uint16_t slot_count) const {
  if (page_buffer_.get()) {
    GetHeader()->slot_count = slot_count;
  }
}

void Page::SetPageType(const uint8_t page_type) const {
  if (page_buffer_.get()) {
    GetHeader()->page_type = page_type;
  }
}

void Page::SetFlags(const uint8_t flags) const {
  if (page_buffer_.get()) {
    GetHeader()->flags = flags;
  }
}

void Page::SetChecksum(const uint32_t checksum) const {
  if (page_buffer_.get()) {
    GetHeader()->checksum = checksum;
  }
}

bool Page::IsDirty() const {
  return page_buffer_.get() ? GetHeader()->is_dirty_ : false;
}

uint16_t Page::GetDeletedTupleCount() const {
  return page_buffer_.get() ? GetHeader()->deleted_tuple_count_ : 0;
}

size_t Page::GetFragmentedBytes() const {
  return page_buffer_.get() ? GetHeader()->fragmented_bytes_ : 0;
}

// Slot directory methods

// Helper method to get slot entry pointer
// Slot N is located at offset: PAGE_SIZE - (N+1) * SLOT_ENTRY_SIZE
// Example: PAGE_SIZE = 8192, SLOT_ENTRY_SIZE = 8
//   Slot 0 -> 8192 - (0+1)*8 = 8184
//   Slot 1 -> 8192 - (1+1)*8 = 8176
//   Slot 2 -> 8192 - (2+1)*8 = 8168
SlotEntry* Page::GetSlotEntryPtr(const slot_id_t slot_id) const {
  if (page_buffer_.get() == nullptr) {
    return nullptr;
  }

  auto* page_data = reinterpret_cast<uint8_t*>(page_buffer_.get());
  const size_t slot_offset = PAGE_SIZE - (slot_id + 1) * SLOT_ENTRY_SIZE;
  return reinterpret_cast<SlotEntry*>(page_data + slot_offset);
}

SlotEntry& Page::GetSlotEntry(const slot_id_t slot_id) const {
  return *GetSlotEntryPtr(slot_id);
}

slot_id_t Page::AddSlot(const uint16_t offset, const uint16_t length) const {
  if (page_buffer_.get() == nullptr) {
    LOG_ERROR("Page::AddSlot: Page buffer is null");
    return static_cast<slot_id_t>(-1);
  }

  PageHeader* header = GetHeader();

  // Get current slot count
  const slot_id_t new_slot_id = header->slot_count;

  // Check if there's space for a new slot
  // Slots grow downward from PAGE_SIZE
  // Example calculation: PAGE_SIZE = 8192 (8KB), SLOT_ENTRY_SIZE = 8,
  // for new_slot_id = 0 -> new_slot_offset = 8192 - (0 + 1) * 8 = 8184
  const size_t new_slot_offset =
      PAGE_SIZE - (new_slot_id + 1) * SLOT_ENTRY_SIZE;
  if (new_slot_offset <= header->free_start) {
    // Not enough space
    LOG_WARNING_STREAM("Page::AddSlot: Not enough space on page "
                       << header->page_id
                       << " (free_start: " << header->free_start
                       << ", new_slot_offset: " << new_slot_offset << ")");
    return static_cast<slot_id_t>(-1);
  }

  // Get pointer to new slot
  SlotEntry* slot_entry = GetSlotEntryPtr(new_slot_id);
  if (slot_entry == nullptr) {
    LOG_ERROR_STREAM("Page::AddSlot: Failed to get slot entry pointer for slot "
                     << new_slot_id);
    return static_cast<slot_id_t>(-1);
  }

  // Initialize slot entry
  slot_entry->offset = offset;
  slot_entry->length = length;
  slot_entry->flags = SLOT_VALID;
  slot_entry->next_ptr[0] = 0;
  slot_entry->next_ptr[1] = 0;
  slot_entry->next_ptr[2] = 0;

  // Update slot count and free_End
  header->slot_count++;
  header->free_end = new_slot_offset;

  LOG_INFO_STREAM("Page::AddSlot: Added slot "
                  << new_slot_id << " to page " << header->page_id
                  << " (offset: " << offset << ", length: " << length << ")");

  return new_slot_id;
}

void Page::MarkSlotDeleted(slot_id_t slot_id) const {
  if (page_buffer_ == nullptr || slot_id >= GetHeader()->slot_count) {
    return;
  }

  if (SlotEntry* slot_entry = GetSlotEntryPtr(slot_id)) {
    slot_entry->flags &= ~SLOT_VALID;  // Clear VALID flag
  }
}

bool Page::IsSlotValid(const slot_id_t slot_id) const {
  if (page_buffer_ == nullptr || slot_id >= GetHeader()->slot_count) {
    return false;
  }

  const SlotEntry* slot_entry = GetSlotEntryPtr(slot_id);
  if (slot_entry == nullptr) {
    return false;
  }

  return (slot_entry->flags & SLOT_VALID) != 0;
}

bool Page::IsSlotForwarded(const slot_id_t slot_id) const {
  if (page_buffer_ == nullptr || slot_id >= GetHeader()->slot_count) {
    return false;
  }

  SlotEntry* slot_entry = GetSlotEntryPtr(slot_id);
  if (slot_entry == nullptr) {
    return false;
  }

  return (slot_entry->flags & SLOT_FORWARDED) != 0;
}

TupleId Page::GetForwardingPointer(const slot_id_t slot_id) const {
  TupleId result = {0, 0};

  if (page_buffer_ == nullptr || slot_id >= GetHeader()->slot_count) {
    return result;
  }

  const SlotEntry* slot_entry = GetSlotEntryPtr(slot_id);
  if (slot_entry == nullptr) {
    return result;
  }

  // Decode 24-bit pointer: 16-bit page_id + 8-bit slot_id
  // next_ptr[0] = low byte of page_id
  // next_ptr[1] = high byte of page_id
  // next_ptr[2] = slot_id
  // Example: next_ptr = [0xD2, 0x04, 0x2A] (1234 in decimal for page_id, 42 for
  // slot_id)
  //   page_id = 0xD2 | (0x04 << 8) = 0xD2 | 0x400 = 0x04D2 = 1234
  //   slot_id = 0x2A = 42
  result.page_id = static_cast<page_id_t>(slot_entry->next_ptr[0]) |
                   (static_cast<page_id_t>(slot_entry->next_ptr[1]) << 8);
  result.slot_id = static_cast<slot_id_t>(slot_entry->next_ptr[2]);

  return result;
}

void Page::SetForwardingPointer(const slot_id_t slot_id,
                                const page_id_t target_page_id,
                                const slot_id_t target_slot_id) const {
  if (page_buffer_ == nullptr || slot_id >= GetHeader()->slot_count) {
    return;
  }

  SlotEntry* slot_entry = GetSlotEntryPtr(slot_id);
  if (slot_entry == nullptr) {
    return;
  }

  // Encode 24-bit pointer: 16-bit page_id + 8-bit slot_id
  // Example: target_page_id = 1234 (0x04D2), target_slot_id = 42 (0x2A)
  //   next_ptr[0] = 0x04D2 & 0xFF = 0xD2 (low byte)
  //   next_ptr[1] = (0x04D2 >> 8) & 0xFF = 0x04 (high byte)
  //   next_ptr[2] = 0x2A & 0xFF = 0x2A
  //   Result: [0xD2, 0x04, 0x2A]
  slot_entry->next_ptr[0] = static_cast<uint8_t>(target_page_id & 0xFF);
  slot_entry->next_ptr[1] = static_cast<uint8_t>((target_page_id >> 8) & 0xFF);
  slot_entry->next_ptr[2] = static_cast<uint8_t>(target_slot_id & 0xFF);

  // Set FORWARDED flag
  slot_entry->flags |= SLOT_FORWARDED;
}

slot_id_t Page::InsertTuple(const char* tuple_data, uint16_t tuple_size) const {
  // Input validation
  if (page_buffer_.get() == nullptr) {
    LOG_ERROR("Page::InsertTuple: Page buffer is null");
    return INVALID_SLOT_ID;
  }

  if (tuple_data == nullptr) {
    LOG_ERROR("Page::InsertTuple: Tuple data is null");
    return INVALID_SLOT_ID;
  }

  if (tuple_size == 0) {
    LOG_ERROR("Page::InsertTuple: Tuple size is zero");
    return INVALID_SLOT_ID;
  }

  PageHeader* header = GetHeader();

  // Try to find a deleted slot to reuse first
  slot_id_t slot_id = FindDeletedSlot();
  size_t required_space;

  if (slot_id != INVALID_SLOT_ID) {
    // Reusing deleted slot - only need space for tuple data
    required_space = tuple_size;
    LOG_INFO_STREAM("Page::InsertTuple: Found deleted slot "
                    << slot_id << " to reuse on page " << header->page_id);
  } else {
    // Need new slot - require space for tuple data + slot entry
    required_space = tuple_size + SLOT_ENTRY_SIZE;
  }

  // Check if we have enough space
  if (size_t available_space = GetAvailableFreeSpace();
      available_space < required_space) {
    LOG_WARNING_STREAM("Page::InsertTuple: Insufficient space on page "
                       << header->page_id << " (required: " << required_space
                       << ", available: " << available_space << ")");
    return INVALID_SLOT_ID;
  }

  // Get current free_start offset for writing tuple data
  const uint16_t tuple_offset = header->free_start;

  // Allocate or reuse slot
  if (slot_id == INVALID_SLOT_ID) {
    // No deleted slot found, create a new one
    slot_id = AddSlot(tuple_offset, tuple_size);
    if (slot_id == INVALID_SLOT_ID) {
      LOG_ERROR_STREAM("Page::InsertTuple: Failed to add new slot on page "
                       << header->page_id);
      return INVALID_SLOT_ID;
    }
  } else {
    // Reuse the deleted slot - update its entry
    SlotEntry* slot_entry = GetSlotEntryPtr(slot_id);
    if (slot_entry == nullptr) {
      LOG_ERROR_STREAM("Page::InsertTuple: Failed to get slot entry for slot "
                       << slot_id);
      return INVALID_SLOT_ID;
    }

    // Save old length BEFORE overwriting - needed to decrement fragmentation
    // stats
    uint16_t old_length = slot_entry->length;

    // Update the slot entry for reuse
    slot_entry->offset = tuple_offset;
    slot_entry->length = tuple_size;
    slot_entry->flags = SLOT_VALID;  // Mark as valid, clear other flags
    slot_entry->next_ptr[0] = 0;
    slot_entry->next_ptr[1] = 0;
    slot_entry->next_ptr[2] = 0;

    // Decrement fragmentation counters - we're reclaiming a deleted slot
    header->deleted_tuple_count_--;
    header->fragmented_bytes_ -= old_length;
  }

  // Write tuple data to page
  auto* page_data = reinterpret_cast<uint8_t*>(page_buffer_.get());
  std::memcpy(page_data + tuple_offset, tuple_data, tuple_size);

  // Update page header - increment free_start
  header->free_start += tuple_size;

  // Recompute and update checksum
  const uint32_t new_checksum = ComputeChecksum();
  header->checksum = new_checksum;

  LOG_INFO_STREAM("Page::InsertTuple: Successfully inserted tuple of size "
                  << tuple_size << " at slot " << slot_id << " on page "
                  << header->page_id
                  << " (new free_start: " << header->free_start << ")");

  // Return the slot_id where tuple was inserted
  return slot_id;
}

void Page::CompactPage() const {
  // validate compaction
  if (GetHeader()->deleted_tuple_count_ == 0) {
    return;  // Nothing to compact
  }

  if (GetHeader()->slot_count == GetHeader()->deleted_tuple_count_) {
    // All tuples deleted
    GetHeader()->free_start = sizeof(PageHeader);
    GetHeader()->slot_count = 0;
    GetHeader()->deleted_tuple_count_ = 0;
    GetHeader()->fragmented_bytes_ = 0;
    const uint32_t checksum = ComputeChecksum();
    GetHeader()->checksum = checksum;
    return;  // All space reclaimed
  }

  std::vector<TupleInfo> valid_tuples;
  uint16_t expected_valid =
      GetHeader()->slot_count > GetHeader()->deleted_tuple_count_
          ? GetHeader()->slot_count - GetHeader()->deleted_tuple_count_
          : 0;
  valid_tuples.reserve(expected_valid);

  for (slot_id_t i = 0; i < GetHeader()->slot_count; i++) {
    SlotEntry slot = GetSlotEntry(i);

    if (slot.flags & SLOT_VALID) {
      TupleInfo info{};
      info.original_slot_id = i;
      info.offset = slot.offset;
      info.length = slot.length;
      info.flags = slot.flags;
      info.next_ptr[0] = slot.next_ptr[0];
      info.next_ptr[1] = slot.next_ptr[1];
      info.next_ptr[2] = slot.next_ptr[2];

      valid_tuples.push_back(info);
    }
  }

  // Allocate temporary buffer for compacted data
  const size_t header_size = sizeof(PageHeader);
  const size_t buffer_size = GetHeader()->free_start - header_size;
  const auto temp_buffer = std::make_unique<uint8_t[]>(buffer_size);
  memset(temp_buffer.get(), 0, buffer_size);

  size_t new_offset = 0;  // Offset in temp buffer
  auto* page_data = reinterpret_cast<uint8_t*>(page_buffer_.get());

  for (auto& info : valid_tuples) {
    // Copy tuple data
    memcpy(temp_buffer.get() + new_offset, page_data + info.offset,
           info.length);

    // Update offset in info structure
    info.offset =
        header_size +
        new_offset;  // Offset relative to start of page (after header)

    // Advance offset
    new_offset += info.length;
  }

  // Copy compacted data back to page (after page header)
  memcpy(page_data + header_size, temp_buffer.get(), new_offset);

  // Update slot offsets WITHOUT renumbering (preserves external forwarding
  // pointers)
  for (const auto& info : valid_tuples) {
    SlotEntry& slot = GetSlotEntry(info.original_slot_id);
    slot.offset = info.offset;
    slot.length = info.length;
  }

  // Clear deleted slots by marking them invalid
  for (slot_id_t i = 0; i < GetHeader()->slot_count; i++) {
    SlotEntry* slot = GetSlotEntryPtr(i);
    if (slot && !(slot->flags & SLOT_VALID)) {
      slot->offset = 0;
      slot->length = 0;
      slot->flags = 0;
      slot->next_ptr[0] = 0;
      slot->next_ptr[1] = 0;
      slot->next_ptr[2] = 0;
    }
  }

  // Update page header
  GetHeader()->free_start = header_size + new_offset;
  GetHeader()->deleted_tuple_count_ = 0;
  GetHeader()->fragmented_bytes_ = 0;

  // Recompute checksum
  const uint32_t checksum = ComputeChecksum();
  GetHeader()->checksum = checksum;

  LOG_INFO_STREAM("Page::CompactPage: Compacted page "
                  << GetHeader()->page_id << ", reclaimed "
                  << (buffer_size - new_offset) << " bytes, "
                  << "new free_start: " << GetHeader()->free_start);
}

size_t Page::GetAvailableFreeSpace() const {
  const PageHeader* header = GetHeader();
  uint16_t free_start = header->free_start;
  uint16_t free_end = header->free_end;

  if (free_end < free_start) {
    LOG_ERROR_STREAM("Page::GetAvailableFreeSpace: Invalid free space pointers "
                     << "(free_start: " << free_start
                     << ", free_end: " << free_end << ")");
    return 0;
  }

  return free_end - free_start;
}

slot_id_t Page::FindDeletedSlot() const {
  const PageHeader* header = GetHeader();
  const slot_id_t slot_count = header->slot_count;

  // Iterate through all existing slots to find an invalid (deleted) one
  for (slot_id_t slot_id = 0; slot_id < slot_count; ++slot_id) {
    if (!IsSlotValid(slot_id)) {
      LOG_INFO_STREAM("Page::FindDeletedSlot: Found deleted slot "
                      << slot_id << " on page " << header->page_id);
      return slot_id;
    }
  }

  // No deleted slots found
  return INVALID_SLOT_ID;
}

void Page::RecomputeFragmentationStats() const {
  GetHeader()->deleted_tuple_count_ = 0;
  GetHeader()->fragmented_bytes_ = 0;

  // Scan slot directory
  for (slot_id_t i = 0; i < GetHeader()->slot_count; i++) {
    if (const SlotEntry slot = GetSlotEntry(i); !(slot.flags & SLOT_VALID)) {
      GetHeader()->deleted_tuple_count_++;
      GetHeader()->fragmented_bytes_ += slot.length;
    }
  }
}

ErrorCode Page::DeleteTuple(uint16_t slot_id) const {
  if (slot_id >= GetSlotCount()) {
    LOG_ERROR_STREAM("Page::DeleteTuple: Invalid slot id " << slot_id);
    return ErrorCode{-1, "Page::DeleteTuple: Invalid slot id "};
  }

  // Slot N at offset: 8192 - (N + 1) * 8
  SlotEntry* slot_entry = GetSlotEntryPtr(slot_id);
  if (slot_entry == nullptr) {
    LOG_ERROR_STREAM("Page::DeleteTuple: Invalid slot id " << slot_id);
    return ErrorCode{-1, "Page::DeleteTuple: Invalid slot id "};
  }

  if (!(slot_entry->flags & SLOT_VALID)) {
    LOG_ERROR_STREAM("Page::DeleteTuple: Tuple already deleted " << slot_id);
    return ErrorCode{-2, "Page::DeleteTuple: Tuple already deleted"};
  }

  slot_entry->flags &= ~SLOT_VALID;
  GetHeader()->deleted_tuple_count_++;
  GetHeader()->fragmented_bytes_ += slot_entry->length;
  GetHeader()->is_dirty_ = true;

  // Recompute checksum after modifying page
  const uint32_t checksum = ComputeChecksum();
  GetHeader()->checksum = checksum;

  return ErrorCode{0, "Page::DeleteTuple"};
}

bool Page::ShouldCompact() const {
  // No point compacting if no deletions
  if (GetHeader()->deleted_tuple_count_ == 0) {
    return false;
  }

  // High fragmentation ratio (>= 50%)
  if (const size_t used_space = GetHeader()->free_start - sizeof(PageHeader);
      used_space > 0 &&
      GetHeader()->fragmented_bytes_ * 100 / used_space >= 50) {
    return true;
  }

  // Many deleted slots (>= 50%)
  if (GetHeader()->deleted_tuple_count_ * 2 >= GetHeader()->slot_count) {
    return true;
  }

  // Insertion would fail but compaction would help
  const size_t available = GetHeader()->free_end - GetHeader()->free_start;
  const size_t potentially_available =
      available + GetHeader()->fragmented_bytes_;

  if (available < 100 && potentially_available >= 100) {
    // Can't fit small tuple now, but could after compaction
    return true;
  }

  return false;
}

ErrorCode Page::UpdateTupleInPlace(slot_id_t slot_id, const char* new_data,
                                   uint16_t new_size) const {
  if (page_buffer_.get() == nullptr) {
    LOG_ERROR("Page::UpdateTupleInPlace: Page buffer is null");
    return ErrorCode{-1, "Page::UpdateTupleInPlace: Page buffer is null"};
  }

  if (new_data == nullptr) {
    LOG_ERROR("Page::UpdateTupleInPlace: New data is null");
    return ErrorCode{-2, "Page::UpdateTupleInPlace: New data is null"};
  }

  if (new_size == 0) {
    LOG_ERROR("Page::UpdateTupleInPlace: New size is zero");
    return ErrorCode{-3, "Page::UpdateTupleInPlace: New size is zero"};
  }

  PageHeader* header = GetHeader();

  // Check if slot_id is valid
  if (slot_id >= header->slot_count) {
    LOG_ERROR_STREAM("Page::UpdateTupleInPlace: Invalid slot id " << slot_id);
    return ErrorCode{-4, "Page::UpdateTupleInPlace: Invalid slot id " +
                             std::to_string(slot_id)};
  }

  SlotEntry* slot_entry = GetSlotEntryPtr(slot_id);
  if (slot_entry == nullptr) {
    LOG_ERROR_STREAM(
        "Page::UpdateTupleInPlace: Failed to get slot entry for slot "
        << slot_id);
    return ErrorCode{-5, "Page::UpdateTupleInPlace: Failed to get slot entry"};
  }

  if (!(slot_entry->flags & SLOT_VALID)) {
    LOG_ERROR_STREAM("Page::UpdateTupleInPlace: Slot " << slot_id
                                                       << " is not valid");
    return ErrorCode{-6, "Page::UpdateTupleInPlace: Slot is not valid"};
  }

  // Check if slot is forwarded cannot update forwarded slots in place
  if (slot_entry->flags & SLOT_FORWARDED) {
    LOG_ERROR_STREAM("Page::UpdateTupleInPlace: Slot " << slot_id
                                                       << " is forwarded");
    return ErrorCode{-7, "Page::UpdateTupleInPlace: Slot is forwarded"};
  }

  // Check if new size fits in the current slot
  if (new_size > slot_entry->length) {
    LOG_WARNING_STREAM("Page::UpdateTupleInPlace: New size "
                       << new_size << " exceeds current size "
                       << slot_entry->length);
    return ErrorCode{-8,
                     "Page::UpdateTupleInPlace: New size exceeds current size"};
  }

  // Perform in place update
  auto* page_data = reinterpret_cast<uint8_t*>(page_buffer_.get());
  std::memcpy(page_data + slot_entry->offset, new_data, new_size);

  slot_entry->length = new_size;
  header->is_dirty_ = true;
  const uint32_t new_checksum = ComputeChecksum();
  header->checksum = new_checksum;

  LOG_INFO_STREAM(
      "Page::UpdateTupleInPlace: Successfully updated tuple at slot "
      << slot_id << " on page " << header->page_id << " (new size: " << new_size
      << ")");

  return ErrorCode{0, "Page::UpdateTupleInPlace: Success"};
}

ErrorCode Page::MarkSlotForwarded(slot_id_t slot_id, page_id_t target_page_id,
                                  slot_id_t target_slot_id) const {
  if (page_buffer_.get() == nullptr) {
    LOG_ERROR("Page::MarkSlotForwarded: Page buffer is null");
    return ErrorCode{-1, "Page::MarkSlotForwarded: Page buffer is null"};
  }

  PageHeader* header = GetHeader();

  if (slot_id >= header->slot_count) {
    LOG_ERROR_STREAM("Page::MarkSlotForwarded: Invalid slot id " << slot_id);
    return ErrorCode{-2, "Page::MarkSlotForwarded: Invalid slot id " +
                             std::to_string(slot_id)};
  }

  SlotEntry* slot_entry = GetSlotEntryPtr(slot_id);
  if (slot_entry == nullptr) {
    LOG_ERROR_STREAM(
        "Page::MarkSlotForwarded: Failed to get slot entry for slot "
        << slot_id);
    return ErrorCode{-3, "Page::MarkSlotForwarded: Failed to get slot entry"};
  }

  if (!(slot_entry->flags & SLOT_VALID)) {
    LOG_ERROR_STREAM("Page::MarkSlotForwarded: Slot " << slot_id
                                                      << " is not valid");
    return ErrorCode{-4, "Page::MarkSlotForwarded: Slot is not valid"};
  }

  // Mark the tuple data as reclaimed since it's forwarded
  // This allows the space to be reclaimed during compaction
  const uint16_t old_length = slot_entry->length;
  slot_entry->length = 0;
  SetForwardingPointer(slot_id, target_page_id, target_slot_id);
  header->fragmented_bytes_ += old_length;
  header->is_dirty_ = true;

  const uint32_t new_checksum = ComputeChecksum();
  header->checksum = new_checksum;

  LOG_INFO_STREAM("Page::MarkSlotForwarded: Marked slot "
                  << slot_id << " as forwarded to page " << target_page_id
                  << ", slot " << target_slot_id);

  return ErrorCode{0, "Page::MarkSlotForwarded: Success"};
}

TupleId Page::FollowForwardingChain(slot_id_t slot_id, int max_hops) const {
  TupleId result = {0, 0};

  if (page_buffer_.get() == nullptr) {
    LOG_ERROR("Page::FollowForwardingChain: Page buffer is null");
    return result;
  }

  PageHeader* header = GetHeader();

  if (header->slot_count == 0 || slot_id >= header->slot_count) {
    LOG_ERROR_STREAM("Page::FollowForwardingChain: Invalid slot id "
                     << slot_id << " (slot_count: " << header->slot_count
                     << ")");
    return result;
  }

  // Track visited slots to detect circular chains
  struct VisitedSlot {
    page_id_t page_id;
    slot_id_t slot_id;

    bool operator==(const VisitedSlot& other) const {
      return page_id == other.page_id && slot_id == other.slot_id;
    }
  };

  std::vector<VisitedSlot> visited;
  visited.reserve(max_hops);

  page_id_t current_page_id = header->page_id;
  slot_id_t current_slot_id = slot_id;

  // Follow the chain allow up to max_hops + 1 iterations
  // so we can check the final destination after following max_hops
  for (int hop = 0; hop <= max_hops; ++hop) {
    // Check for circular reference
    VisitedSlot current_visit = {current_page_id, current_slot_id};
    for (const auto& visited_slot : visited) {
      if (visited_slot == current_visit) {
        // Circular chain detected
        LOG_WARNING_STREAM(
            "Page::FollowForwardingChain: Circular chain detected at page "
            << current_page_id << ", slot " << current_slot_id);
        result.page_id = 0;
        result.slot_id = 0;
        return result;
      }
    }

    visited.push_back(current_visit);

    // Check if current slot is on this page
    if (current_page_id != header->page_id) {
      result.page_id = current_page_id;
      result.slot_id = current_slot_id;
      LOG_INFO_STREAM(
          "Page::FollowForwardingChain: Followed chain to different page "
          << current_page_id << ", slot " << current_slot_id);
      return result;
    }

    // Get the slot entry
    if (current_slot_id >= header->slot_count) {
      LOG_ERROR_STREAM("Page::FollowForwardingChain: Invalid slot id in chain "
                       << current_slot_id);
      result.page_id = 0;
      result.slot_id = 0;
      return result;
    }

    SlotEntry* slot_entry = GetSlotEntryPtr(current_slot_id);
    if (slot_entry == nullptr) {
      LOG_ERROR_STREAM(
          "Page::FollowForwardingChain: Failed to get slot entry for slot "
          << current_slot_id);
      result.page_id = 0;
      result.slot_id = 0;
      return result;
    }

    if (!(slot_entry->flags & SLOT_VALID)) {
      LOG_ERROR_STREAM("Page::FollowForwardingChain: Slot " << current_slot_id
                                                            << " is not valid");
      result.page_id = 0;
      result.slot_id = 0;
      return result;
    }

    if (!(slot_entry->flags & SLOT_FORWARDED)) {
      result.page_id = current_page_id;
      result.slot_id = current_slot_id;
      LOG_INFO_STREAM(
          "Page::FollowForwardingChain: Found final destination at page "
          << current_page_id << ", slot " << current_slot_id << " after " << hop
          << " hops");
      return result;
    }

    if (hop >= max_hops) {
      // Reached max_hops without finding a non-forwarded slot
      LOG_WARNING_STREAM("Page::FollowForwardingChain: Reached max hops ("
                         << max_hops << ")");
      result.page_id = 0;
      result.slot_id = 0;
      return result;
    }

    TupleId next = GetForwardingPointer(current_slot_id);
    current_page_id = next.page_id;
    current_slot_id = next.slot_id;
  }

  result.page_id = 0;
  result.slot_id = 0;
  return result;
}
