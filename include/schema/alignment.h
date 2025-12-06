//
// Created by Abhi on 01/12/25.
//

#ifndef STORAGEENGINE_ALIGNMENT_H
#define STORAGEENGINE_ALIGNMENT_H

#include <cstddef>
#include <cstdint>

enum DataType {
  // 1
  BOOLEAN,
  TINYINT,
  CHAR,
  // 2
  SMALLINT,
  // 4
  INTEGER,
  FLOAT,
  // 8
  BIGINT,
  DOUBLE
};

namespace alignment {
size_t CalculateAlignment(DataType type);
size_t CalculatePadding(size_t current_offset, size_t alignment);
size_t AlignOffset(size_t offset, DataType type);
size_t GetFixedSize(DataType type, size_t size_param);
}  // namespace alignment

#endif  // STORAGEENGINE_ALIGNMENT_H
