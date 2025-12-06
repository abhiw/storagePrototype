//
// Created by Abhi on 29/11/25.
//

#include "../include/common/types.h"

#include <gtest/gtest.h>

#include "../include/common/config.h"

TEST(ConstTest, ConstValueTest) {
  EXPECT_EQ(PAGE_SIZE, 8192);
  EXPECT_EQ(INVALID_PAGE_ID, 0);
  EXPECT_EQ(INVALID_SLOT_ID, 65535);
  EXPECT_EQ(INVALID_FRAME_ID, -1);
}

TEST(TypesTest, DataTypeTest) {
  static_assert(sizeof(page_id_t) == sizeof(uint32_t),
                "page_id_t must be uint32_t");
  static_assert(sizeof(slot_id_t) == sizeof(uint16_t),
                "slot_id_t must be uint16_t");
  static_assert(sizeof(frame_id_t) == sizeof(uint32_t),
                "frame_id_t must be uint32_t");
}