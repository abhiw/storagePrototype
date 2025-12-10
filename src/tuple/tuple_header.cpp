#include "../../include/tuple/tuple_header.h"

#include <cassert>
#include <cstring>

TupleHeader::TupleHeader(uint16_t field_count, uint16_t var_field_count)
    : null_bitmap_(0),
      field_count_(field_count),
      var_field_count_(var_field_count) {
  assert(field_count <= 64 &&
         "Field count exceeds 64-bit null bitmap capacity");

  if (var_field_count > 0) {
    var_offsets_.resize(var_field_count, 0);
  }
}

void TupleHeader::SetFieldNull(uint16_t field_index, bool is_null) {
  assert(field_index < field_count_ && "Field index out of bounds");

  // Bitwise operation: Set or clear bit at position field_index
  // Example: field_index=3, is_null=true
  //   mask = 1ULL << 3 = 0b00001000
  //   null_bitmap_ |= mask sets bit 3
  // Example: field_index=3, is_null=false
  //   mask = ~(1ULL << 3) = 0b11110111
  //   null_bitmap_ &= mask clears bit 3
  if (is_null) {
    null_bitmap_ |= (1ULL << field_index);
  } else {
    null_bitmap_ &= ~(1ULL << field_index);
  }
}

bool TupleHeader::IsFieldNull(uint16_t field_index) const {
  assert(field_index < field_count_ && "Field index out of bounds");

  // Bitwise operation: Check if bit at position field_index is set
  // Example: field_index=3, null_bitmap_=0b00001010
  //   mask = 1ULL << 3 = 0b00001000
  //   null_bitmap_ & mask = 0b00001000 (non-zero, true)
  // Example: field_index=5
  //   mask = 1ULL << 5 = 0b00100000
  //   null_bitmap_ & mask = 0b00000000 (zero, false)
  return (null_bitmap_ & (1ULL << field_index)) != 0;
}

void TupleHeader::SetVariableLengthOffset(uint16_t var_field_index,
                                          uint16_t offset) {
  assert(var_field_index < var_field_count_ &&
         "Variable field index out of bounds");
  var_offsets_[var_field_index] = offset;
}

uint16_t TupleHeader::GetVariableLengthOffset(uint16_t var_field_index) const {
  assert(var_field_index < var_field_count_ &&
         "Variable field index out of bounds");
  return var_offsets_[var_field_index];
}

size_t TupleHeader::CalculateHeaderSize(uint16_t var_field_count) {
  // Header layout: [8-byte null_bitmap][uint16_t offset_0]...[uint16_t
  // offset_N-1] Size = 8 + (var_field_count * 2) Round up to 8-byte alignment
  // Example: var_field_count=0  -> size=8,  aligned=8
  // Example: var_field_count=1  -> size=10, aligned=16
  // Example: var_field_count=3  -> size=14, aligned=16
  // Example: var_field_count=4  -> size=16, aligned=16
  size_t size = 8 + (var_field_count * sizeof(uint16_t));
  return (size + 7) / 8 * 8;
}

size_t TupleHeader::GetHeaderSize() const {
  return CalculateHeaderSize(var_field_count_);
}

void TupleHeader::SerializeTo(char* buffer) const {
  // Pointer arithmetic: Write null_bitmap at offset 0
  std::memcpy(buffer, &null_bitmap_, sizeof(null_bitmap_));

  // Pointer arithmetic: Write variable-length offsets starting at offset 8
  // Example: var_field_count=3
  //   buffer+8 points to byte 8
  //   Write 3 uint16_t values (6 bytes) at buffer[8..13]
  if (var_field_count_ > 0) {
    std::memcpy(buffer + 8, var_offsets_.data(),
                var_field_count_ * sizeof(uint16_t));
  }
}

TupleHeader TupleHeader::DeserializeFrom(const char* buffer,
                                         uint16_t field_count,
                                         uint16_t var_field_count) {
  TupleHeader header(field_count, var_field_count);

  // Pointer arithmetic: Read null_bitmap from offset 0
  std::memcpy(&header.null_bitmap_, buffer, sizeof(header.null_bitmap_));

  // Pointer arithmetic: Read variable-length offsets from offset 8
  if (var_field_count > 0) {
    std::memcpy(header.var_offsets_.data(), buffer + 8,
                var_field_count * sizeof(uint16_t));
  }

  return header;
}
