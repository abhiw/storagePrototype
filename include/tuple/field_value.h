#ifndef STORAGEENGINE_FIELD_VALUE_H
#define STORAGEENGINE_FIELD_VALUE_H

#include <cstdint>
#include <string>
#include <vector>

#include "../common/types.h"

class FieldValue {
 private:
  DataType type_;
  bool is_null_;

  union {
    bool boolean_val;
    int8_t tinyint_val;
    int16_t smallint_val;
    int32_t integer_val;
    int64_t bigint_val;
    float float_val;
    double double_val;
  } data_;

  std::string string_val_;
  std::vector<uint8_t> blob_val_;

 public:
  static FieldValue Null(DataType type);
  static FieldValue Boolean(bool value);
  static FieldValue TinyInt(int8_t value);
  static FieldValue SmallInt(int16_t value);
  static FieldValue Integer(int32_t value);
  static FieldValue BigInt(int64_t value);
  static FieldValue Float(float value);
  static FieldValue Double(double value);
  static FieldValue Char(const std::string& value);
  static FieldValue VarChar(const std::string& value);
  static FieldValue Text(const std::string& value);
  static FieldValue Blob(const std::vector<uint8_t>& value);

  bool IsNull() const { return is_null_; }
  DataType GetType() const { return type_; }

  bool GetBoolean() const;
  int8_t GetTinyInt() const;
  int16_t GetSmallInt() const;
  int32_t GetInteger() const;
  int64_t GetBigInt() const;
  float GetFloat() const;
  double GetDouble() const;
  const std::string& GetString() const;
  const std::vector<uint8_t>& GetBlob() const;

  size_t GetSerializedSize() const;

 private:
  FieldValue(DataType type, bool is_null);
};

#endif
