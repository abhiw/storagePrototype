//
// Created by Abhi on 29/11/25.
//

#ifndef STORAGEENGINE_TYPES_H
#define STORAGEENGINE_TYPES_H

#include <memory>
#include <string>

#define page_id_t uint32_t
#define slot_id_t uint16_t
#define frame_id_t uint32_t

enum PageType {
  DATA_PAGE,   // 0
  INDEX_PAGE,  // 1
  FSM_PAGE     // 2
};

enum DataType {
  BOOLEAN,   // 0
  TINYINT,   // 1
  SMALLINT,  // 2
  INTEGER,   // 3
  BIGINT,    // 4
  FLOAT,     // 5
  DOUBLE,    // 6
  CHAR,      // 7
  VARCHAR,   // 8
  TEXT,      // 9
  BLOB       // 10
};

struct TupleId {
  page_id_t page_id;
  slot_id_t slot_id;
};

// Custom deleter for aligned memory allocated with std::aligned_alloc
// Must use std::free() instead of delete because aligned_alloc uses malloc
// family

struct AlignedDeleter {
  void operator()(char* ptr) const {
    if (ptr) {
      std::free(ptr);
    }
  }
};

// Type alias for aligned buffer with custom deleter
// Used for page buffers that require specific alignment (e.g., 8192-byte
// alignment)

using AlignedBuffer = std::unique_ptr<char[], AlignedDeleter>;

struct ErrorCode {
  int code;
  std::string message;
};

#endif  // STORAGEENGINE_TYPES_H