#include "../../include/tuple/tuple_serializer.h"

#include <cstring>
#include <stdexcept>

#include "../../include/schema/alignment.h"

size_t TupleSerializer::SerializeFixedLength(
    const Schema& schema, const std::vector<FieldValue>& values, char* buffer,
    size_t buffer_size) {
  if (!schema.IsFinalized()) {
    throw std::runtime_error("Schema must be finalized before serialization");
  }

  if (!schema.IsFixedLength()) {
    throw std::runtime_error(
        "Use SerializeVariableLength for variable-length schemas");
  }

  if (values.size() != schema.GetColumnCount()) {
    throw std::runtime_error("Value count does not match column count");
  }

  size_t field_count = schema.GetColumnCount();

  TupleHeader header(field_count, 0);
  size_t header_size = header.GetHeaderSize();

  if (buffer_size < header_size) {
    throw std::runtime_error("Buffer too small for tuple header");
  }

  std::memset(buffer, 0, buffer_size);

  size_t current_offset = header_size;

  // Serialize fixed-length fields with proper alignment
  // Algorithm:
  // 1. For each column in order:
  //    a. Align current_offset for this field's type
  //    b. If field is null, set null bit and skip writing data
  //    c. Otherwise, write field value at aligned offset
  //    d. Advance offset by field size
  for (size_t i = 0; i < field_count; i++) {
    ColumnDefinition col = schema.GetColumn(i);
    DataType type = col.GetDataType();

    // Align offset for this field's type
    // Example: current_offset=9, type=INTEGER (4-byte alignment)
    //   aligned_offset = AlignOffset(9, INTEGER) = 12
    //   Padding of 3 bytes inserted at offsets 9,10,11
    current_offset = alignment::AlignOffset(current_offset, type);

    if (values[i].IsNull()) {
      header.SetFieldNull(i, true);
      // Skip writing data for null fields
    } else {
      // Pointer arithmetic: Calculate destination address
      // buffer + current_offset points to the aligned location
      char* dest = buffer + current_offset;

      // Write value based on type using memcpy
      switch (type) {
        case DataType::BOOLEAN: {
          bool val = values[i].GetBoolean();
          std::memcpy(dest, &val, sizeof(bool));
          break;
        }
        case DataType::TINYINT: {
          int8_t val = values[i].GetTinyInt();
          std::memcpy(dest, &val, sizeof(int8_t));
          break;
        }
        case DataType::SMALLINT: {
          int16_t val = values[i].GetSmallInt();
          std::memcpy(dest, &val, sizeof(int16_t));
          break;
        }
        case DataType::INTEGER: {
          int32_t val = values[i].GetInteger();
          std::memcpy(dest, &val, sizeof(int32_t));
          break;
        }
        case DataType::BIGINT: {
          int64_t val = values[i].GetBigInt();
          std::memcpy(dest, &val, sizeof(int64_t));
          break;
        }
        case DataType::FLOAT: {
          float val = values[i].GetFloat();
          std::memcpy(dest, &val, sizeof(float));
          break;
        }
        case DataType::DOUBLE: {
          double val = values[i].GetDouble();
          std::memcpy(dest, &val, sizeof(double));
          break;
        }
        case DataType::CHAR: {
          const std::string& str = values[i].GetString();
          size_t char_size = col.GetFixedSize();
          if (str.length() > char_size) {
            throw std::runtime_error("CHAR value exceeds fixed size");
          }
          std::memcpy(dest, str.c_str(), str.length());
          break;
        }
        default:
          throw std::runtime_error(
              "Unexpected variable-length type in fixed-length schema");
      }
    }

    current_offset += col.GetFixedSize();
  }

  header.SerializeTo(buffer);

  return current_offset;
}

std::vector<FieldValue> TupleSerializer::DeserializeFixedLength(
    const Schema& schema, const char* buffer, size_t buffer_size) {
  if (!schema.IsFinalized()) {
    throw std::runtime_error("Schema must be finalized before deserialization");
  }

  if (!schema.IsFixedLength()) {
    throw std::runtime_error(
        "Use DeserializeVariableLength for variable-length schemas");
  }

  size_t field_count = schema.GetColumnCount();
  TupleHeader header = TupleHeader::DeserializeFrom(buffer, field_count, 0);
  size_t header_size = header.GetHeaderSize();

  if (buffer_size < header_size) {
    throw std::runtime_error("Buffer too small for tuple header");
  }

  std::vector<FieldValue> result;
  result.reserve(field_count);

  size_t current_offset = header_size;

  for (size_t i = 0; i < field_count; i++) {
    ColumnDefinition col = schema.GetColumn(i);
    DataType type = col.GetDataType();

    current_offset = alignment::AlignOffset(current_offset, type);

    if (header.IsFieldNull(i)) {
      result.push_back(FieldValue::Null(type));
    } else {
      const char* src = buffer + current_offset;

      switch (type) {
        case DataType::BOOLEAN: {
          bool val;
          std::memcpy(&val, src, sizeof(bool));
          result.push_back(FieldValue::Boolean(val));
          break;
        }
        case DataType::TINYINT: {
          int8_t val;
          std::memcpy(&val, src, sizeof(int8_t));
          result.push_back(FieldValue::TinyInt(val));
          break;
        }
        case DataType::SMALLINT: {
          int16_t val;
          std::memcpy(&val, src, sizeof(int16_t));
          result.push_back(FieldValue::SmallInt(val));
          break;
        }
        case DataType::INTEGER: {
          int32_t val;
          std::memcpy(&val, src, sizeof(int32_t));
          result.push_back(FieldValue::Integer(val));
          break;
        }
        case DataType::BIGINT: {
          int64_t val;
          std::memcpy(&val, src, sizeof(int64_t));
          result.push_back(FieldValue::BigInt(val));
          break;
        }
        case DataType::FLOAT: {
          float val;
          std::memcpy(&val, src, sizeof(float));
          result.push_back(FieldValue::Float(val));
          break;
        }
        case DataType::DOUBLE: {
          double val;
          std::memcpy(&val, src, sizeof(double));
          result.push_back(FieldValue::Double(val));
          break;
        }
        case DataType::CHAR: {
          size_t char_size = col.GetFixedSize();
          std::string str(src, char_size);
          size_t null_pos = str.find('\0');
          if (null_pos != std::string::npos) {
            str = str.substr(0, null_pos);
          }
          result.push_back(FieldValue::Char(str));
          break;
        }
        default:
          throw std::runtime_error(
              "Unexpected variable-length type in fixed-length schema");
      }
    }

    current_offset += col.GetFixedSize();
  }

  return result;
}

size_t TupleSerializer::SerializeVariableLength(
    const Schema& schema, const std::vector<FieldValue>& values, char* buffer,
    size_t buffer_size) {
  if (!schema.IsFinalized()) {
    throw std::runtime_error("Schema must be finalized before serialization");
  }

  if (values.size() != schema.GetColumnCount()) {
    throw std::runtime_error("Value count does not match column count");
  }

  size_t field_count = schema.GetColumnCount();
  uint16_t var_field_count = 0;

  for (size_t i = 0; i < field_count; i++) {
    if (!schema.GetColumn(i).IsFixedLength()) {
      var_field_count++;
    }
  }

  TupleHeader header(field_count, var_field_count);
  size_t header_size = header.GetHeaderSize();

  if (buffer_size < header_size) {
    throw std::runtime_error("Buffer too small for tuple header");
  }

  std::memset(buffer, 0, buffer_size);

  size_t current_offset = header_size;
  uint16_t var_field_index = 0;

  // Serialize fixed-length fields first
  for (size_t i = 0; i < field_count; i++) {
    ColumnDefinition col = schema.GetColumn(i);
    DataType type = col.GetDataType();

    if (col.IsFixedLength()) {
      current_offset = alignment::AlignOffset(current_offset, type);

      if (values[i].IsNull()) {
        header.SetFieldNull(i, true);
      } else {
        char* dest = buffer + current_offset;

        switch (type) {
          case DataType::BOOLEAN: {
            bool val = values[i].GetBoolean();
            std::memcpy(dest, &val, sizeof(bool));
            break;
          }
          case DataType::TINYINT: {
            int8_t val = values[i].GetTinyInt();
            std::memcpy(dest, &val, sizeof(int8_t));
            break;
          }
          case DataType::SMALLINT: {
            int16_t val = values[i].GetSmallInt();
            std::memcpy(dest, &val, sizeof(int16_t));
            break;
          }
          case DataType::INTEGER: {
            int32_t val = values[i].GetInteger();
            std::memcpy(dest, &val, sizeof(int32_t));
            break;
          }
          case DataType::BIGINT: {
            int64_t val = values[i].GetBigInt();
            std::memcpy(dest, &val, sizeof(int64_t));
            break;
          }
          case DataType::FLOAT: {
            float val = values[i].GetFloat();
            std::memcpy(dest, &val, sizeof(float));
            break;
          }
          case DataType::DOUBLE: {
            double val = values[i].GetDouble();
            std::memcpy(dest, &val, sizeof(double));
            break;
          }
          default:
            throw std::runtime_error(
                "Unexpected type in fixed-length serialization");
        }
      }

      current_offset += col.GetFixedSize();
    }
  }

  // Align to 8-byte boundary before variable-length data
  // Example: current_offset=53 -> aligned to 56
  current_offset = (current_offset + 7) / 8 * 8;

  // Serialize variable-length fields
  // Format: [2-byte length][data bytes]
  // Example: VARCHAR "Hello" -> [0x05, 0x00, 'H', 'e', 'l', 'l', 'o']
  for (size_t i = 0; i < field_count; i++) {
    ColumnDefinition col = schema.GetColumn(i);

    if (!col.IsFixedLength()) {
      if (values[i].IsNull()) {
        header.SetFieldNull(i, true);
        header.SetVariableLengthOffset(var_field_index, 0xFFFF);
      } else {
        header.SetVariableLengthOffset(var_field_index, current_offset);

        DataType type = col.GetDataType();
        if (type == DataType::VARCHAR || type == DataType::TEXT ||
            type == DataType::CHAR) {
          const std::string& str = values[i].GetString();
          uint16_t length = static_cast<uint16_t>(str.length());

          if (current_offset + sizeof(uint16_t) + length > buffer_size) {
            throw std::runtime_error(
                "Buffer too small for variable-length data");
          }

          // Pointer arithmetic: Write length prefix
          std::memcpy(buffer + current_offset, &length, sizeof(uint16_t));
          current_offset += sizeof(uint16_t);

          // Pointer arithmetic: Write string data
          if (length > 0) {
            std::memcpy(buffer + current_offset, str.c_str(), length);
            current_offset += length;
          }
        } else if (type == DataType::BLOB) {
          const std::vector<uint8_t>& blob = values[i].GetBlob();
          uint16_t length = static_cast<uint16_t>(blob.size());

          if (current_offset + sizeof(uint16_t) + length > buffer_size) {
            throw std::runtime_error(
                "Buffer too small for variable-length data");
          }

          std::memcpy(buffer + current_offset, &length, sizeof(uint16_t));
          current_offset += sizeof(uint16_t);

          if (length > 0) {
            std::memcpy(buffer + current_offset, blob.data(), length);
            current_offset += length;
          }
        }
      }
      var_field_index++;
    }
  }

  header.SerializeTo(buffer);

  return current_offset;
}

std::vector<FieldValue> TupleSerializer::DeserializeVariableLength(
    const Schema& schema, const char* buffer, size_t buffer_size) {
  if (!schema.IsFinalized()) {
    throw std::runtime_error("Schema must be finalized before deserialization");
  }

  size_t field_count = schema.GetColumnCount();
  uint16_t var_field_count = 0;

  for (size_t i = 0; i < field_count; i++) {
    if (!schema.GetColumn(i).IsFixedLength()) {
      var_field_count++;
    }
  }

  TupleHeader header =
      TupleHeader::DeserializeFrom(buffer, field_count, var_field_count);
  size_t header_size = header.GetHeaderSize();

  if (buffer_size < header_size) {
    throw std::runtime_error("Buffer too small for tuple header");
  }

  std::vector<FieldValue> result;
  result.reserve(field_count);

  size_t current_offset = header_size;
  uint16_t var_field_index = 0;

  for (size_t i = 0; i < field_count; i++) {
    ColumnDefinition col = schema.GetColumn(i);
    DataType type = col.GetDataType();

    if (col.IsFixedLength()) {
      current_offset = alignment::AlignOffset(current_offset, type);

      if (header.IsFieldNull(i)) {
        result.push_back(FieldValue::Null(type));
      } else {
        const char* src = buffer + current_offset;

        switch (type) {
          case DataType::BOOLEAN: {
            bool val;
            std::memcpy(&val, src, sizeof(bool));
            result.push_back(FieldValue::Boolean(val));
            break;
          }
          case DataType::TINYINT: {
            int8_t val;
            std::memcpy(&val, src, sizeof(int8_t));
            result.push_back(FieldValue::TinyInt(val));
            break;
          }
          case DataType::SMALLINT: {
            int16_t val;
            std::memcpy(&val, src, sizeof(int16_t));
            result.push_back(FieldValue::SmallInt(val));
            break;
          }
          case DataType::INTEGER: {
            int32_t val;
            std::memcpy(&val, src, sizeof(int32_t));
            result.push_back(FieldValue::Integer(val));
            break;
          }
          case DataType::BIGINT: {
            int64_t val;
            std::memcpy(&val, src, sizeof(int64_t));
            result.push_back(FieldValue::BigInt(val));
            break;
          }
          case DataType::FLOAT: {
            float val;
            std::memcpy(&val, src, sizeof(float));
            result.push_back(FieldValue::Float(val));
            break;
          }
          case DataType::DOUBLE: {
            double val;
            std::memcpy(&val, src, sizeof(double));
            result.push_back(FieldValue::Double(val));
            break;
          }
          default:
            throw std::runtime_error(
                "Unexpected type in fixed-length deserialization");
        }
      }

      current_offset += col.GetFixedSize();
    } else {
      if (header.IsFieldNull(i)) {
        result.push_back(FieldValue::Null(type));
      } else {
        uint16_t offset = header.GetVariableLengthOffset(var_field_index);
        if (offset == 0xFFFF) {
          result.push_back(FieldValue::Null(type));
        } else {
          const char* src = buffer + offset;

          if (type == DataType::VARCHAR || type == DataType::TEXT ||
              type == DataType::CHAR) {
            uint16_t length;
            std::memcpy(&length, src, sizeof(uint16_t));
            src += sizeof(uint16_t);

            std::string str(src, length);
            if (type == DataType::CHAR) {
              result.push_back(FieldValue::Char(str));
            } else if (type == DataType::VARCHAR) {
              result.push_back(FieldValue::VarChar(str));
            } else {
              result.push_back(FieldValue::Text(str));
            }
          } else if (type == DataType::BLOB) {
            uint16_t length;
            std::memcpy(&length, src, sizeof(uint16_t));
            src += sizeof(uint16_t);

            std::vector<uint8_t> blob(src, src + length);
            result.push_back(FieldValue::Blob(blob));
          }
        }
      }
      var_field_index++;
    }
  }

  return result;
}

size_t TupleSerializer::CalculateSerializedSize(
    const Schema& schema, const std::vector<FieldValue>& values) {
  if (!schema.IsFinalized()) {
    throw std::runtime_error("Schema must be finalized");
  }

  size_t field_count = schema.GetColumnCount();
  uint16_t var_field_count = 0;

  for (size_t i = 0; i < field_count; i++) {
    if (!schema.GetColumn(i).IsFixedLength()) {
      var_field_count++;
    }
  }

  size_t size = TupleHeader::CalculateHeaderSize(var_field_count);

  for (size_t i = 0; i < field_count; i++) {
    ColumnDefinition col = schema.GetColumn(i);

    if (col.IsFixedLength()) {
      size = alignment::AlignOffset(size, col.GetDataType());
      size += col.GetFixedSize();
    } else {
      if (!values[i].IsNull()) {
        size += values[i].GetSerializedSize();
      }
    }
  }

  return size;
}
