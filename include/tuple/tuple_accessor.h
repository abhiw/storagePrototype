#ifndef STORAGEENGINE_TUPLE_ACCESSOR_H
#define STORAGEENGINE_TUPLE_ACCESSOR_H

#include <string>
#include <vector>

#include "../schema/schema.h"
#include "field_value.h"
#include "tuple_header.h"

class TupleAccessor {
 public:
  TupleAccessor(const Schema& schema, const char* buffer, size_t buffer_size);

  TupleAccessor(const TupleAccessor&) = delete;
  TupleAccessor& operator=(const TupleAccessor&) = delete;
  TupleAccessor(TupleAccessor&&) = delete;
  TupleAccessor& operator=(TupleAccessor&&) = delete;

  bool IsNull(const std::string& column_name) const;
  bool IsNull(size_t field_index) const;

  bool GetBoolean(const std::string& column_name) const;
  int8_t GetTinyInt(const std::string& column_name) const;
  int16_t GetSmallInt(const std::string& column_name) const;
  int32_t GetInteger(const std::string& column_name) const;
  int64_t GetBigInt(const std::string& column_name) const;
  float GetFloat(const std::string& column_name) const;
  double GetDouble(const std::string& column_name) const;
  std::string GetString(const std::string& column_name) const;
  std::vector<uint8_t> GetBlob(const std::string& column_name) const;

  bool GetBoolean(size_t field_index) const;
  int8_t GetTinyInt(size_t field_index) const;
  int16_t GetSmallInt(size_t field_index) const;
  int32_t GetInteger(size_t field_index) const;
  int64_t GetBigInt(size_t field_index) const;
  float GetFloat(size_t field_index) const;
  double GetDouble(size_t field_index) const;
  std::string GetString(size_t field_index) const;
  std::vector<uint8_t> GetBlob(size_t field_index) const;

  FieldValue GetFieldValue(const std::string& column_name) const;
  FieldValue GetFieldValue(size_t field_index) const;

 private:
  const Schema& schema_;
  const char* buffer_;
  size_t buffer_size_;
  TupleHeader header_;
  mutable std::vector<FieldValue> cached_values_;
  mutable bool is_deserialized_;

  void EnsureDeserialized() const;
  void ValidateColumnName(const std::string& name,
                          DataType expected_type) const;
  void ValidateFieldIndex(size_t index, DataType expected_type) const;
  size_t GetFieldIndex(const std::string& column_name) const;
};

#endif
