#include "../../include/tuple/field_value.h"

#include <cstring>
#include <stdexcept>

#include "../../include/schema/alignment.h"

FieldValue::FieldValue(DataType type, bool is_null)
    : type_(type), is_null_(is_null) {
  std::memset(&data_, 0, sizeof(data_));
}

FieldValue FieldValue::Null(DataType type) { return FieldValue(type, true); }

FieldValue FieldValue::Boolean(bool value) {
  FieldValue fv(DataType::BOOLEAN, false);
  fv.data_.boolean_val = value;
  return fv;
}

FieldValue FieldValue::TinyInt(int8_t value) {
  FieldValue fv(DataType::TINYINT, false);
  fv.data_.tinyint_val = value;
  return fv;
}

FieldValue FieldValue::SmallInt(int16_t value) {
  FieldValue fv(DataType::SMALLINT, false);
  fv.data_.smallint_val = value;
  return fv;
}

FieldValue FieldValue::Integer(int32_t value) {
  FieldValue fv(DataType::INTEGER, false);
  fv.data_.integer_val = value;
  return fv;
}

FieldValue FieldValue::BigInt(int64_t value) {
  FieldValue fv(DataType::BIGINT, false);
  fv.data_.bigint_val = value;
  return fv;
}

FieldValue FieldValue::Float(float value) {
  FieldValue fv(DataType::FLOAT, false);
  fv.data_.float_val = value;
  return fv;
}

FieldValue FieldValue::Double(double value) {
  FieldValue fv(DataType::DOUBLE, false);
  fv.data_.double_val = value;
  return fv;
}

FieldValue FieldValue::Char(const std::string& value) {
  FieldValue fv(DataType::CHAR, false);
  fv.string_val_ = value;
  return fv;
}

FieldValue FieldValue::VarChar(const std::string& value) {
  FieldValue fv(DataType::VARCHAR, false);
  fv.string_val_ = value;
  return fv;
}

FieldValue FieldValue::Text(const std::string& value) {
  FieldValue fv(DataType::TEXT, false);
  fv.string_val_ = value;
  return fv;
}

FieldValue FieldValue::Blob(const std::vector<uint8_t>& value) {
  FieldValue fv(DataType::BLOB, false);
  fv.blob_val_ = value;
  return fv;
}

bool FieldValue::GetBoolean() const {
  if (is_null_) {
    throw std::runtime_error("Cannot read NULL value");
  }
  if (type_ != DataType::BOOLEAN) {
    throw std::runtime_error("Type mismatch: expected BOOLEAN");
  }
  return data_.boolean_val;
}

int8_t FieldValue::GetTinyInt() const {
  if (is_null_) {
    throw std::runtime_error("Cannot read NULL value");
  }
  if (type_ != DataType::TINYINT) {
    throw std::runtime_error("Type mismatch: expected TINYINT");
  }
  return data_.tinyint_val;
}

int16_t FieldValue::GetSmallInt() const {
  if (is_null_) {
    throw std::runtime_error("Cannot read NULL value");
  }
  if (type_ != DataType::SMALLINT) {
    throw std::runtime_error("Type mismatch: expected SMALLINT");
  }
  return data_.smallint_val;
}

int32_t FieldValue::GetInteger() const {
  if (is_null_) {
    throw std::runtime_error("Cannot read NULL value");
  }
  if (type_ != DataType::INTEGER) {
    throw std::runtime_error("Type mismatch: expected INTEGER");
  }
  return data_.integer_val;
}

int64_t FieldValue::GetBigInt() const {
  if (is_null_) {
    throw std::runtime_error("Cannot read NULL value");
  }
  if (type_ != DataType::BIGINT) {
    throw std::runtime_error("Type mismatch: expected BIGINT");
  }
  return data_.bigint_val;
}

float FieldValue::GetFloat() const {
  if (is_null_) {
    throw std::runtime_error("Cannot read NULL value");
  }
  if (type_ != DataType::FLOAT) {
    throw std::runtime_error("Type mismatch: expected FLOAT");
  }
  return data_.float_val;
}

double FieldValue::GetDouble() const {
  if (is_null_) {
    throw std::runtime_error("Cannot read NULL value");
  }
  if (type_ != DataType::DOUBLE) {
    throw std::runtime_error("Type mismatch: expected DOUBLE");
  }
  return data_.double_val;
}

const std::string& FieldValue::GetString() const {
  if (is_null_) {
    throw std::runtime_error("Cannot read NULL value");
  }
  if (type_ != DataType::CHAR && type_ != DataType::VARCHAR &&
      type_ != DataType::TEXT) {
    throw std::runtime_error("Type mismatch: expected string type");
  }
  return string_val_;
}

const std::vector<uint8_t>& FieldValue::GetBlob() const {
  if (is_null_) {
    throw std::runtime_error("Cannot read NULL value");
  }
  if (type_ != DataType::BLOB) {
    throw std::runtime_error("Type mismatch: expected BLOB");
  }
  return blob_val_;
}

size_t FieldValue::GetSerializedSize() const {
  if (is_null_) {
    return 0;
  }

  size_t fixed_size = alignment::GetFixedSize(type_, 0);
  if (fixed_size > 0) {
    return fixed_size;
  }

  switch (type_) {
    case DataType::CHAR:
    case DataType::VARCHAR:
    case DataType::TEXT:
      return sizeof(uint16_t) + string_val_.length();
    case DataType::BLOB:
      return sizeof(uint16_t) + blob_val_.size();
    default:
      return 0;
  }
}
