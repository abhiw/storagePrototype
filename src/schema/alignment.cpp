//
// Created by Abhi on 01/12/25.
//

#include "../../include/schema/alignment.h"

namespace alignment {

size_t CalculateAlignment(const DataType type) {
  switch (type) {
    case BOOLEAN:
    case TINYINT:
    case CHAR:
      return 1;
    case SMALLINT:
      return 2;
    case INTEGER:
    case FLOAT:
      return 4;
    case BIGINT:
    case DOUBLE:
      return 8;
  }
  return 0;
}

size_t CalculatePadding(size_t current_offset, size_t alignment_value) {
  // Handle edge case: avoid division by zero
  if (alignment_value == 0) {
    return 0;
  }

  // Calculate padding needed to align current_offset to the alignment boundary
  // Formula: (alignment_value - (current_offset % alignment_value)) %
  // alignment_value
  //
  // Examples:
  //   current_offset = 3, alignment = 4:
  //     (4 - (3 % 4)) % 4 = (4 - 3) % 4 = 1
  //     Need 1 byte of padding to reach offset 4
  //
  //   current_offset = 4, alignment = 4:
  //     (4 - (4 % 4)) % 4 = (4 - 0) % 4 = 0
  //     Already aligned, no padding needed
  //
  //   current_offset = 5, alignment = 8:
  //     (8 - (5 % 8)) % 8 = (8 - 5) % 8 = 3
  //     Need 3 bytes of padding to reach offset 8
  return (alignment_value - (current_offset % alignment_value)) %
         alignment_value;
}

size_t AlignOffset(size_t offset, DataType type) {
  // Get the alignment requirement for this data type
  size_t align_value = CalculateAlignment(type);

  // Calculate padding needed to align the offset
  size_t padding_needed = CalculatePadding(offset, align_value);

  // Return the aligned offset
  // Example: offset = 5, type = INTEGER (alignment = 4)
  //   padding_needed = 3, aligned_offset = 5 + 3 = 8
  return offset + padding_needed;
}

size_t GetFixedSize(DataType type, size_t size_param) {
  // Determine fixed size based on DataType for fixed-length types.
  // For CHAR, treat as fixed-length only when a positive size_param is
  // provided.
  switch (type) {
    case BOOLEAN:
    case TINYINT:
      return size_t(1);
    case SMALLINT:
      return size_t(2);
    case INTEGER:
    case FLOAT:
      return size_t(4);
    case BIGINT:
    case DOUBLE:
      return size_t(8);
    case CHAR:
      // If user provided a positive size_param, treat CHAR as fixed-length
      // of that size; otherwise it's variable-length (e.g., VARCHAR-style).
      if (size_param > 0) {
        return size_param;
      }
      return 0;
    default:
      return 0;
  }
}

}  // namespace alignment