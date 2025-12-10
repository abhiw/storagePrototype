#include "../../include/tuple/tuple_builder.h"

#include <stdexcept>

TupleBuilder::TupleBuilder(const Schema& schema) : schema_(schema) {
  if (!schema_.IsFinalized()) {
    throw std::runtime_error("Schema must be finalized");
  }
  values_.resize(schema_.GetColumnCount());
}

size_t TupleBuilder::GetFieldIndex(const std::string& column_name) const {
  if (!schema_.HasColumn(column_name)) {
    throw std::runtime_error("Column not found: " + column_name);
  }
  return schema_.GetColumn(column_name).GetFieldIndex();
}

void TupleBuilder::ValidateColumnName(const std::string& name,
                                      DataType expected_type) const {
  if (!schema_.HasColumn(name)) {
    throw std::runtime_error("Column not found: " + name);
  }
  ColumnDefinition col = schema_.GetColumn(name);
  if (col.GetDataType() != expected_type) {
    throw std::runtime_error("Type mismatch for column: " + name);
  }
}

void TupleBuilder::ValidateFieldIndex(size_t index,
                                      DataType expected_type) const {
  if (index >= schema_.GetColumnCount()) {
    throw std::runtime_error("Field index out of bounds");
  }
  ColumnDefinition col = schema_.GetColumn(index);
  if (col.GetDataType() != expected_type) {
    throw std::runtime_error("Type mismatch for field index");
  }
}

void TupleBuilder::ValidateComplete() const {
  for (size_t i = 0; i < schema_.GetColumnCount(); i++) {
    ColumnDefinition col = schema_.GetColumn(i);
    if (!col.GetIsNullable() && !values_[i].has_value()) {
      throw std::runtime_error("Non-nullable field not set: " +
                               col.GetColumnName());
    }
  }
}

TupleBuilder& TupleBuilder::SetNull(const std::string& column_name) {
  size_t index = GetFieldIndex(column_name);
  ColumnDefinition col = schema_.GetColumn(index);
  if (!col.GetIsNullable()) {
    throw std::runtime_error("Cannot set NULL on non-nullable column: " +
                             column_name);
  }
  values_[index] = FieldValue::Null(col.GetDataType());
  return *this;
}

TupleBuilder& TupleBuilder::SetBoolean(const std::string& column_name,
                                       bool value) {
  ValidateColumnName(column_name, DataType::BOOLEAN);
  size_t index = GetFieldIndex(column_name);
  values_[index] = FieldValue::Boolean(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetTinyInt(const std::string& column_name,
                                       int8_t value) {
  ValidateColumnName(column_name, DataType::TINYINT);
  size_t index = GetFieldIndex(column_name);
  values_[index] = FieldValue::TinyInt(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetSmallInt(const std::string& column_name,
                                        int16_t value) {
  ValidateColumnName(column_name, DataType::SMALLINT);
  size_t index = GetFieldIndex(column_name);
  values_[index] = FieldValue::SmallInt(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetInteger(const std::string& column_name,
                                       int32_t value) {
  ValidateColumnName(column_name, DataType::INTEGER);
  size_t index = GetFieldIndex(column_name);
  values_[index] = FieldValue::Integer(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetBigInt(const std::string& column_name,
                                      int64_t value) {
  ValidateColumnName(column_name, DataType::BIGINT);
  size_t index = GetFieldIndex(column_name);
  values_[index] = FieldValue::BigInt(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetFloat(const std::string& column_name,
                                     float value) {
  ValidateColumnName(column_name, DataType::FLOAT);
  size_t index = GetFieldIndex(column_name);
  values_[index] = FieldValue::Float(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetDouble(const std::string& column_name,
                                      double value) {
  ValidateColumnName(column_name, DataType::DOUBLE);
  size_t index = GetFieldIndex(column_name);
  values_[index] = FieldValue::Double(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetChar(const std::string& column_name,
                                    const std::string& value) {
  ValidateColumnName(column_name, DataType::CHAR);
  size_t index = GetFieldIndex(column_name);
  values_[index] = FieldValue::Char(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetVarChar(const std::string& column_name,
                                       const std::string& value) {
  ValidateColumnName(column_name, DataType::VARCHAR);
  size_t index = GetFieldIndex(column_name);
  values_[index] = FieldValue::VarChar(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetText(const std::string& column_name,
                                    const std::string& value) {
  ValidateColumnName(column_name, DataType::TEXT);
  size_t index = GetFieldIndex(column_name);
  values_[index] = FieldValue::Text(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetBlob(const std::string& column_name,
                                    const std::vector<uint8_t>& value) {
  ValidateColumnName(column_name, DataType::BLOB);
  size_t index = GetFieldIndex(column_name);
  values_[index] = FieldValue::Blob(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetNull(size_t field_index) {
  if (field_index >= schema_.GetColumnCount()) {
    throw std::runtime_error("Field index out of bounds");
  }
  ColumnDefinition col = schema_.GetColumn(field_index);
  if (!col.GetIsNullable()) {
    throw std::runtime_error("Cannot set NULL on non-nullable column");
  }
  values_[field_index] = FieldValue::Null(col.GetDataType());
  return *this;
}

TupleBuilder& TupleBuilder::SetBoolean(size_t field_index, bool value) {
  ValidateFieldIndex(field_index, DataType::BOOLEAN);
  values_[field_index] = FieldValue::Boolean(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetTinyInt(size_t field_index, int8_t value) {
  ValidateFieldIndex(field_index, DataType::TINYINT);
  values_[field_index] = FieldValue::TinyInt(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetSmallInt(size_t field_index, int16_t value) {
  ValidateFieldIndex(field_index, DataType::SMALLINT);
  values_[field_index] = FieldValue::SmallInt(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetInteger(size_t field_index, int32_t value) {
  ValidateFieldIndex(field_index, DataType::INTEGER);
  values_[field_index] = FieldValue::Integer(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetBigInt(size_t field_index, int64_t value) {
  ValidateFieldIndex(field_index, DataType::BIGINT);
  values_[field_index] = FieldValue::BigInt(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetFloat(size_t field_index, float value) {
  ValidateFieldIndex(field_index, DataType::FLOAT);
  values_[field_index] = FieldValue::Float(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetDouble(size_t field_index, double value) {
  ValidateFieldIndex(field_index, DataType::DOUBLE);
  values_[field_index] = FieldValue::Double(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetChar(size_t field_index,
                                    const std::string& value) {
  ValidateFieldIndex(field_index, DataType::CHAR);
  values_[field_index] = FieldValue::Char(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetVarChar(size_t field_index,
                                       const std::string& value) {
  ValidateFieldIndex(field_index, DataType::VARCHAR);
  values_[field_index] = FieldValue::VarChar(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetText(size_t field_index,
                                    const std::string& value) {
  ValidateFieldIndex(field_index, DataType::TEXT);
  values_[field_index] = FieldValue::Text(value);
  return *this;
}

TupleBuilder& TupleBuilder::SetBlob(size_t field_index,
                                    const std::vector<uint8_t>& value) {
  ValidateFieldIndex(field_index, DataType::BLOB);
  values_[field_index] = FieldValue::Blob(value);
  return *this;
}

std::vector<FieldValue> TupleBuilder::Build() const {
  ValidateComplete();

  std::vector<FieldValue> result;
  result.reserve(values_.size());

  for (const auto& opt_value : values_) {
    if (opt_value.has_value()) {
      result.push_back(*opt_value);
    } else {
      size_t index = &opt_value - &values_[0];
      result.push_back(
          FieldValue::Null(schema_.GetColumn(index).GetDataType()));
    }
  }

  return result;
}

void TupleBuilder::Reset() {
  values_.clear();
  values_.resize(schema_.GetColumnCount());
}
