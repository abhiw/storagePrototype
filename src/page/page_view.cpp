#include "../../include/page/page_view.h"

#include <cstring>

#include "../../include/common/checksum.h"
#include "../../include/common/config.h"
#include "../../include/common/logger.h"

uint32_t PageView::ComputeChecksum() const {
  if (page_buffer_ == nullptr) {
    return 0;
  }

  // PageHeader layout (total size: 40 bytes):
  //   Bytes  0-11: On-disk persistent fields (included)  Bytes  0-11
  //   Bytes 12-15: checksum field (excluded) Bytes 12-15
  //   Bytes 16-39: Runtime metadata (excluded) Bytes 16-39
  //   Bytes 40-8191: Page data (included) Bytes 40-8191
  //
  //   Runtime fields (deleted_tuple_count_, fragmented_bytes_, is_dirty_)
  //   are computed at runtime and should NOT affect the checksum. They are
  //   zeroed before WritePage() and recomputed after ReadPage().
  const auto* page_data = reinterpret_cast<const uint8_t*>(page_buffer_);
  const size_t checksum_offset = offsetof(PageHeader, checksum);
  const size_t checksum_size = sizeof(PageHeader::checksum);

  // Initialize CRC
  uint32_t result_crc = checksum::Init();

  // Checksum persistent header fields (bytes 0-11)
  result_crc = checksum::Update(result_crc, page_data, checksum_offset);

  // Skip checksum field (bytes 12-15) - treat as zeros to avoid circular
  // dependency
  const uint32_t zero_checksum = 0;
  result_crc = checksum::Update(
      result_crc, reinterpret_cast<const uint8_t*>(&zero_checksum),
      checksum_size);

  // Skip runtime metadata fields (bytes 16-39)
  // Checksum page data area (bytes 40-8191)
  constexpr size_t remaining_offset =
      sizeof(PageHeader);  // Start after entire PageHeader (40 bytes)
  constexpr size_t remaining_size =
      PAGE_SIZE - remaining_offset;  // 8192 - 40 = 8152 bytes
  result_crc = checksum::Update(result_crc, page_data + remaining_offset,
                                remaining_size);

  // Finalize CRC
  return checksum::Finalize(result_crc);
}

bool PageView::VerifyChecksum() const {
  if (page_buffer_ == nullptr) {
    LOG_ERROR("PageView::VerifyChecksum: Page buffer is null");
    return false;
  }

  PageHeader* header = GetHeader();
  uint32_t computed = ComputeChecksum();
  bool valid = (header->checksum == computed);

  if (!valid) {
    LOG_ERROR_STREAM("PageView::VerifyChecksum: Checksum mismatch for page "
                     << header->page_id << " (expected: " << header->checksum
                     << ", computed: " << computed << ")");
  }

  return valid;
}

// Getters
uint16_t PageView::GetPageId() const {
  return page_buffer_ ? GetHeader()->page_id : 0;
}

uint16_t PageView::GetSlotId() const {
  return page_buffer_ ? GetHeader()->slot_id : 0;
}

uint16_t PageView::GetFreeStart() const {
  return page_buffer_ ? GetHeader()->free_start : 0;
}

uint16_t PageView::GetFreeEnd() const {
  return page_buffer_ ? GetHeader()->free_end : 0;
}

uint16_t PageView::GetSlotCount() const {
  return page_buffer_ ? GetHeader()->slot_count : 0;
}

uint8_t PageView::GetPageType() const {
  return page_buffer_ ? GetHeader()->page_type : 0;
}

uint8_t PageView::GetFlags() const {
  return page_buffer_ ? GetHeader()->flags : 0;
}

uint32_t PageView::GetChecksum() const {
  return page_buffer_ ? GetHeader()->checksum : 0;
}

bool PageView::IsDirty() const {
  return page_buffer_ ? GetHeader()->is_dirty_ : false;
}

size_t PageView::GetFragmentedBytes() const {
  return page_buffer_ ? GetHeader()->fragmented_bytes_ : 0;
}

// Setters
void PageView::SetPageId(uint16_t page_id) const {
  if (page_buffer_) {
    GetHeader()->page_id = page_id;
  }
}

void PageView::SetSlotId(uint16_t slot_id) const {
  if (page_buffer_) {
    GetHeader()->slot_id = slot_id;
  }
}

void PageView::SetFreeStart(const uint16_t free_start) const {
  if (page_buffer_) {
    GetHeader()->free_start = free_start;
  }
}

void PageView::SetFreeEnd(const uint16_t free_end) const {
  if (page_buffer_) {
    GetHeader()->free_end = free_end;
  }
}

void PageView::SetSlotCount(const uint16_t slot_count) const {
  if (page_buffer_) {
    GetHeader()->slot_count = slot_count;
  }
}

void PageView::SetPageType(const uint8_t page_type) const {
  if (page_buffer_) {
    GetHeader()->page_type = page_type;
  }
}

void PageView::SetFlags(const uint8_t flags) const {
  if (page_buffer_) {
    GetHeader()->flags = flags;
  }
}

void PageView::SetChecksum(const uint32_t checksum) const {
  if (page_buffer_) {
    GetHeader()->checksum = checksum;
  }
}
