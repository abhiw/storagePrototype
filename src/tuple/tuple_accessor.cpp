#include "../../include/tuple/tuple_accessor.h"

#include <stdexcept>

#include "../../include/tuple/tuple_serializer.h"

TupleAccessor::TupleAccessor(const Schema& schema, const char* buffer,
                             size_t buffer_size)
    : schema_(schema),
      buffer_(buffer),
      buffer_size_(buffer_size),
      header_(0, 0),
      is_deserialized_(false) {
  if (!schema_.IsFinalized()) {
    throw std::runtime_error("Schema must be finalized");
  }

  uint16_t var_field_count = 0;
  for (size_t i = 0; i < schema_.GetColumnCount(); i++) {
    if (!schema_.GetColumn(i).IsFixedLength()) {
      var_field_count++;
    }
  }

  header_ = TupleHeader::DeserializeFrom(buffer, schema_.GetColumnCount(),
                                         var_field_count);
}

void TupleAccessor::EnsureDeserialized() const {
  if (!is_deserialized_) {
    if (schema_.IsFixedLength()) {
      cached_values_ = TupleSerializer::DeserializeFixedLength(schema_, buffer_,
                                                               buffer_size_);
    } else {
      cached_values_ = TupleSerializer::DeserializeVariableLength(
          schema_, buffer_, buffer_size_);
    }
    is_deserialized_ = true;
  }
}

size_t TupleAccessor::GetFieldIndex(const std::string& column_name) const {
  if (!schema_.HasColumn(column_name)) {
    throw std::runtime_error("Column not found: " + column_name);
  }
  return schema_.GetColumn(column_name).GetFieldIndex();
}

void TupleAccessor::ValidateColumnName(const std::string& name,
                                       DataType expected_type) const {
  if (!schema_.HasColumn(name)) {
    throw std::runtime_error("Column not found: " + name);
  }
  ColumnDefinition col = schema_.GetColumn(name);
  if (col.GetDataType() != expected_type) {
    throw std::runtime_error("Type mismatch for column: " + name);
  }
}

void TupleAccessor::ValidateFieldIndex(size_t index,
                                       DataType expected_type) const {
  if (index >= schema_.GetColumnCount()) {
    throw std::runtime_error("Field index out of bounds");
  }
  ColumnDefinition col = schema_.GetColumn(index);
  if (col.GetDataType() != expected_type) {
    throw std::runtime_error("Type mismatch for field index");
  }
}

bool TupleAccessor::IsNull(const std::string& column_name) const {
  size_t index = GetFieldIndex(column_name);
  return header_.IsFieldNull(index);
}

bool TupleAccessor::IsNull(size_t field_index) const {
  if (field_index >= schema_.GetColumnCount()) {
    throw std::runtime_error("Field index out of bounds");
  }
  return header_.IsFieldNull(field_index);
}

bool TupleAccessor::GetBoolean(const std::string& column_name) const {
  ValidateColumnName(column_name, DataType::BOOLEAN);
  size_t index = GetFieldIndex(column_name);
  return GetBoolean(index);
}

bool TupleAccessor::GetBoolean(size_t field_index) const {
  ValidateFieldIndex(field_index, DataType::BOOLEAN);
  EnsureDeserialized();
  return cached_values_[field_index].GetBoolean();
}

int8_t TupleAccessor::GetTinyInt(const std::string& column_name) const {
  ValidateColumnName(column_name, DataType::TINYINT);
  size_t index = GetFieldIndex(column_name);
  return GetTinyInt(index);
}

int8_t TupleAccessor::GetTinyInt(size_t field_index) const {
  ValidateFieldIndex(field_index, DataType::TINYINT);
  EnsureDeserialized();
  return cached_values_[field_index].GetTinyInt();
}

int16_t TupleAccessor::GetSmallInt(const std::string& column_name) const {
  ValidateColumnName(column_name, DataType::SMALLINT);
  size_t index = GetFieldIndex(column_name);
  return GetSmallInt(index);
}

int16_t TupleAccessor::GetSmallInt(size_t field_index) const {
  ValidateFieldIndex(field_index, DataType::SMALLINT);
  EnsureDeserialized();
  return cached_values_[field_index].GetSmallInt();
}

int32_t TupleAccessor::GetInteger(const std::string& column_name) const {
  ValidateColumnName(column_name, DataType::INTEGER);
  size_t index = GetFieldIndex(column_name);
  return GetInteger(index);
}

int32_t TupleAccessor::GetInteger(size_t field_index) const {
  ValidateFieldIndex(field_index, DataType::INTEGER);
  EnsureDeserialized();
  return cached_values_[field_index].GetInteger();
}

int64_t TupleAccessor::GetBigInt(const std::string& column_name) const {
  ValidateColumnName(column_name, DataType::BIGINT);
  size_t index = GetFieldIndex(column_name);
  return GetBigInt(index);
}

int64_t TupleAccessor::GetBigInt(size_t field_index) const {
  ValidateFieldIndex(field_index, DataType::BIGINT);
  EnsureDeserialized();
  return cached_values_[field_index].GetBigInt();
}

float TupleAccessor::GetFloat(const std::string& column_name) const {
  ValidateColumnName(column_name, DataType::FLOAT);
  size_t index = GetFieldIndex(column_name);
  return GetFloat(index);
}

float TupleAccessor::GetFloat(size_t field_index) const {
  ValidateFieldIndex(field_index, DataType::FLOAT);
  EnsureDeserialized();
  return cached_values_[field_index].GetFloat();
}

double TupleAccessor::GetDouble(const std::string& column_name) const {
  ValidateColumnName(column_name, DataType::DOUBLE);
  size_t index = GetFieldIndex(column_name);
  return GetDouble(index);
}

double TupleAccessor::GetDouble(size_t field_index) const {
  ValidateFieldIndex(field_index, DataType::DOUBLE);
  EnsureDeserialized();
  return cached_values_[field_index].GetDouble();
}

std::string TupleAccessor::GetString(const std::string& column_name) const {
  size_t index = GetFieldIndex(column_name);
  ColumnDefinition col = schema_.GetColumn(index);
  DataType type = col.GetDataType();
  if (type != DataType::CHAR && type != DataType::VARCHAR &&
      type != DataType::TEXT) {
    throw std::runtime_error("Type mismatch: expected string type");
  }
  return GetString(index);
}

std::string TupleAccessor::GetString(size_t field_index) const {
  if (field_index >= schema_.GetColumnCount()) {
    throw std::runtime_error("Field index out of bounds");
  }
  ColumnDefinition col = schema_.GetColumn(field_index);
  DataType type = col.GetDataType();
  if (type != DataType::CHAR && type != DataType::VARCHAR &&
      type != DataType::TEXT) {
    throw std::runtime_error("Type mismatch: expected string type");
  }
  EnsureDeserialized();
  return cached_values_[field_index].GetString();
}

std::vector<uint8_t> TupleAccessor::GetBlob(
    const std::string& column_name) const {
  ValidateColumnName(column_name, DataType::BLOB);
  size_t index = GetFieldIndex(column_name);
  return GetBlob(index);
}

std::vector<uint8_t> TupleAccessor::GetBlob(size_t field_index) const {
  ValidateFieldIndex(field_index, DataType::BLOB);
  EnsureDeserialized();
  return cached_values_[field_index].GetBlob();
}

FieldValue TupleAccessor::GetFieldValue(const std::string& column_name) const {
  size_t index = GetFieldIndex(column_name);
  return GetFieldValue(index);
}

FieldValue TupleAccessor::GetFieldValue(size_t field_index) const {
  if (field_index >= schema_.GetColumnCount()) {
    throw std::runtime_error("Field index out of bounds");
  }
  EnsureDeserialized();
  return cached_values_[field_index];
}
