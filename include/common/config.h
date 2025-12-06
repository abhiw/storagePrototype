//
// Created by Abhi on 29/11/25.
//

#ifndef STORAGEENGINE_CONFIG_H
#define STORAGEENGINE_CONFIG_H
#include "types.h"

constexpr size_t PAGE_SIZE = 8192;

constexpr int INVALID_PAGE_ID = 0;
constexpr slot_id_t INVALID_SLOT_ID = 65535;
constexpr int INVALID_FRAME_ID = -1;

#endif  // STORAGEENGINE_CONFIG_H