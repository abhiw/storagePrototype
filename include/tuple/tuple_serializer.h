#ifndef STORAGEENGINE_TUPLE_SERIALIZER_H
#define STORAGEENGINE_TUPLE_SERIALIZER_H

#include <vector>

#include "../schema/schema.h"
#include "field_value.h"
#include "tuple_header.h"

class TupleSerializer {
 public:
  static size_t SerializeFixedLength(const Schema& schema,
                                     const std::vector<FieldValue>& values,
                                     char* buffer, size_t buffer_size);

  static std::vector<FieldValue> DeserializeFixedLength(const Schema& schema,
                                                        const char* buffer,
                                                        size_t buffer_size);

  static size_t SerializeVariableLength(const Schema& schema,
                                        const std::vector<FieldValue>& values,
                                        char* buffer, size_t buffer_size);

  static std::vector<FieldValue> DeserializeVariableLength(const Schema& schema,
                                                           const char* buffer,
                                                           size_t buffer_size);

  static size_t CalculateSerializedSize(const Schema& schema,
                                        const std::vector<FieldValue>& values);
};

#endif
