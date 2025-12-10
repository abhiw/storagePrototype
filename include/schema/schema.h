//
// Created by Abhi on 02/12/25.
//

#ifndef STORAGEENGINE_SCHEMA_H
#define STORAGEENGINE_SCHEMA_H

#include <string>
#include <unordered_map>
#include <vector>

#include "../../include/schema/alignment.h"

class ColumnDefinition {
 private:
  std::string column_name_;
  DataType data_type_;
  bool is_nullable_;
  size_t fixed_size_;
  size_t max_size_;
  size_t offset_;
  uint16_t field_index_;

 public:
  ColumnDefinition(std::string column_name, DataType data_type,
                   bool is_nullable, size_t size_param) {
    column_name_ = column_name;
    data_type_ = data_type;
    is_nullable_ = is_nullable;
    field_index_ = 0;
    offset_ = 0;

    size_t determined_fixed = alignment::GetFixedSize(data_type_, size_param);
    if (determined_fixed > 0) {
      fixed_size_ = determined_fixed;
      max_size_ = determined_fixed;
    } else {
      fixed_size_ = 0;
      max_size_ = size_param;
    }
  }
  // Getters and setters for each member
  std::string GetColumnName();
  DataType GetDataType();
  bool GetIsNullable();
  size_t GetFixedSize();
  size_t GetMaxSize();
  size_t GetOffset();
  uint16_t GetFieldIndex();
  void SetFieldIndex(uint16_t index);
  void SetOffset(size_t off);
  void SetFixedSize(size_t size);
  void SetMaxSize(size_t size);
  void SetIsNullable(bool nullable);
  void SetDataType(DataType dt);
  void SetColumnName(const std::string& name);
  bool IsFixedLength();
};

// table_name (string)
// table_id (uint32_t)
// columns (vector<ColumnDefinition>): All columns in order
// is_finalized (bool): Has Finalize() been called?
// is_fixed_length (bool): All columns fixed-length?
// tuple_size (size_t): Total size for fixed-length tuples
// null_bitmap_size (size_t): Bytes needed for null bitmap
// nullable_count (uint16_t): Count of nullable columns
// column_name_to_index (map): Fast lookup by name
class Schema {
 private:
  std::string table_name_;
  uint32_t table_id_;
  std::vector<ColumnDefinition> columns_;
  bool is_finalized_;
  bool is_fixed_length_;
  size_t tuple_size_;
  size_t null_bitmap_size_;
  uint16_t nullable_count_;
  std::unordered_map<std::string, uint16_t> column_name_to_index_;

 public:
  // Constructor
  Schema()
      : table_name_(""),
        table_id_(0),
        is_finalized_(false),
        is_fixed_length_(true),
        tuple_size_(0),
        null_bitmap_size_(0),
        nullable_count_(0) {}

  // AddColumn(name, type, nullable, size);
  void AddColumn(const std::string& name, DataType type, bool is_nullable,
                 size_t size_param);
  void Finalize();
  size_t GetAlignment(DataType type) const;
  size_t GetColumnCount() const;                   // returns columns.size()
  ColumnDefinition GetColumn(size_t index) const;  // returns columns[index]
  ColumnDefinition GetColumn(
      const std::string& name) const;  // lookup in map, return column
  bool HasColumn(const std::string& name) const;  // check map.count(name) > 0
  bool IsFixedLength() const;                     // returns is_fixed_length
  size_t GetTupleSize()
      const;  // returns tuple_size (only valid after Finalize())
  size_t GetNullBitmapSize() const;  // returns null_bitmap_size
  bool IsFinalized() const;          // returns is_finalized
  uint32_t GetTableId() const;
};

#endif  // STORAGEENGINE_SCHEMA_H
