//
// Created by Abhi on 02/12/25.
//

#include "../../include/schema/schema.h"

#include "../../include/schema/alignment.h"

std::string ColumnDefinition::GetColumnName() { return column_name_; }

DataType ColumnDefinition::GetDataType() { return data_type_; }

bool ColumnDefinition::GetIsNullable() { return is_nullable_; }

size_t ColumnDefinition::GetFixedSize() { return fixed_size_; }

size_t ColumnDefinition::GetMaxSize() { return max_size_; }

size_t ColumnDefinition::GetOffset() { return offset_; }

uint16_t ColumnDefinition::GetFieldIndex() { return field_index_; }

void ColumnDefinition::SetFieldIndex(uint16_t index) { field_index_ = index; }

void ColumnDefinition::SetOffset(size_t off) { offset_ = off; }

void ColumnDefinition::SetFixedSize(size_t size) { fixed_size_ = size; }

void ColumnDefinition::SetMaxSize(size_t size) { max_size_ = size; }

void ColumnDefinition::SetIsNullable(bool nullable) { is_nullable_ = nullable; }

void ColumnDefinition::SetDataType(DataType dt) { data_type_ = dt; }

void ColumnDefinition::SetColumnName(const std::string& name) {
  column_name_ = name;
}

bool ColumnDefinition::IsFixedLength() { return this->fixed_size_ > 0; }

void Schema::AddColumn(const std::string& name, DataType type, bool is_nullable,
                       size_t size_param) {
  // Create a new ColumnDefinition
  ColumnDefinition col(name, type, is_nullable, size_param);
  col.SetFieldIndex(columns_.size());
  col.SetFixedSize(size_param);

  // Add to columns vector
  columns_.push_back(col);

  // Add to column_name_to_index map
  column_name_to_index_[name] = columns_.size() - 1;

  // Track nullable columns
  if (is_nullable) {
    nullable_count_++;
  }
}

void Schema::Finalize() {
  if (is_finalized_) {
    return;  // Already finalized
  }

  // Calculate null bitmap size (1 bit per nullable column)
  // Round up to nearest byte
  // Calculate null bitmap size (1 bit per nullable column)
  // Round up to nearest byte
  // Example: 10 nullable columns → (10 + 7) / 8 = 2 bytes
  null_bitmap_size_ = (nullable_count_ + 7) / 8;

  // Calculate tuple size and offsets
  size_t current_offset = null_bitmap_size_;
  bool all_fixed_length = true;

  // Example: 3 columns (INT, DOUBLE, VARCHAR)
  // Offset starts at null_bitmap_size_ (e.g., 1 byte)
  // INT (4 bytes, align 4) → offset 4, next offset 8
  // DOUBLE (8 bytes, align 8) → offset 8, next offset 16
  // VARCHAR (0 bytes, variable) → offset 16, next offset 16
  for (auto& col : columns_) {
    // Align the offset for this column
    current_offset = alignment::AlignOffset(current_offset, col.GetDataType());

    col.SetOffset(current_offset);

    // Update the offset for the next column
    size_t col_size = col.GetFixedSize();
    if (col_size == 0) {
      // Variable length column
      all_fixed_length = false;
      col_size = 0;
    }
    current_offset += col_size;
  }

  is_fixed_length_ = all_fixed_length;
  tuple_size_ = current_offset;
  is_finalized_ = true;
}

size_t Schema::GetAlignment(DataType type) {
  return alignment::CalculateAlignment(type);
}

size_t Schema::GetColumnCount() { return columns_.size(); }

ColumnDefinition Schema::GetColumn(size_t index) { return columns_[index]; }

ColumnDefinition Schema::GetColumn(const std::string& name) {
  if (column_name_to_index_.count(name) > 0) {
    uint16_t index = column_name_to_index_[name];
    return columns_[index];
  }
  // If column not found, return a default ColumnDefinition
  // (This could throw an exception instead)
  return ColumnDefinition("", BOOLEAN, false, 0);
}

bool Schema::HasColumn(const std::string& name) {
  return column_name_to_index_.count(name) > 0;
}

bool Schema::IsFixedLength() { return is_fixed_length_; }

size_t Schema::GetTupleSize() { return tuple_size_; }

size_t Schema::GetNullBitmapSize() { return null_bitmap_size_; }

bool Schema::IsFinalized() { return is_finalized_; }

uint32_t Schema::GetTableId() { return table_id_; }
