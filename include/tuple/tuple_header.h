#ifndef STORAGEENGINE_TUPLE_HEADER_H
#define STORAGEENGINE_TUPLE_HEADER_H

#include <cstddef>
#include <cstdint>
#include <vector>

class TupleHeader {
 public:
  TupleHeader(uint16_t field_count, uint16_t var_field_count);

  void SetFieldNull(uint16_t field_index, bool is_null);
  bool IsFieldNull(uint16_t field_index) const;

  void SetVariableLengthOffset(uint16_t var_field_index, uint16_t offset);
  uint16_t GetVariableLengthOffset(uint16_t var_field_index) const;

  static size_t CalculateHeaderSize(uint16_t var_field_count);
  size_t GetHeaderSize() const;

  void SerializeTo(char* buffer) const;
  static TupleHeader DeserializeFrom(const char* buffer, uint16_t field_count,
                                     uint16_t var_field_count);

  uint16_t GetFieldCount() const { return field_count_; }
  uint16_t GetVarFieldCount() const { return var_field_count_; }

 private:
  uint64_t null_bitmap_;
  uint16_t field_count_;
  uint16_t var_field_count_;
  std::vector<uint16_t> var_offsets_;
};

#endif
