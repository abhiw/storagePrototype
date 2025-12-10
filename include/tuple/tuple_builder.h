#ifndef STORAGEENGINE_TUPLE_BUILDER_H
#define STORAGEENGINE_TUPLE_BUILDER_H

#include <optional>
#include <string>
#include <vector>

#include "../schema/schema.h"
#include "field_value.h"

class TupleBuilder {
 public:
  explicit TupleBuilder(const Schema& schema);

  TupleBuilder& SetNull(const std::string& column_name);
  TupleBuilder& SetBoolean(const std::string& column_name, bool value);
  TupleBuilder& SetTinyInt(const std::string& column_name, int8_t value);
  TupleBuilder& SetSmallInt(const std::string& column_name, int16_t value);
  TupleBuilder& SetInteger(const std::string& column_name, int32_t value);
  TupleBuilder& SetBigInt(const std::string& column_name, int64_t value);
  TupleBuilder& SetFloat(const std::string& column_name, float value);
  TupleBuilder& SetDouble(const std::string& column_name, double value);
  TupleBuilder& SetChar(const std::string& column_name,
                        const std::string& value);
  TupleBuilder& SetVarChar(const std::string& column_name,
                           const std::string& value);
  TupleBuilder& SetText(const std::string& column_name,
                        const std::string& value);
  TupleBuilder& SetBlob(const std::string& column_name,
                        const std::vector<uint8_t>& value);

  TupleBuilder& SetNull(size_t field_index);
  TupleBuilder& SetBoolean(size_t field_index, bool value);
  TupleBuilder& SetTinyInt(size_t field_index, int8_t value);
  TupleBuilder& SetSmallInt(size_t field_index, int16_t value);
  TupleBuilder& SetInteger(size_t field_index, int32_t value);
  TupleBuilder& SetBigInt(size_t field_index, int64_t value);
  TupleBuilder& SetFloat(size_t field_index, float value);
  TupleBuilder& SetDouble(size_t field_index, double value);
  TupleBuilder& SetChar(size_t field_index, const std::string& value);
  TupleBuilder& SetVarChar(size_t field_index, const std::string& value);
  TupleBuilder& SetText(size_t field_index, const std::string& value);
  TupleBuilder& SetBlob(size_t field_index, const std::vector<uint8_t>& value);

  std::vector<FieldValue> Build() const;
  void Reset();

 private:
  const Schema& schema_;
  std::vector<std::optional<FieldValue>> values_;

  void ValidateColumnName(const std::string& name,
                          DataType expected_type) const;
  void ValidateFieldIndex(size_t index, DataType expected_type) const;
  void ValidateComplete() const;
  size_t GetFieldIndex(const std::string& column_name) const;
};

#endif
