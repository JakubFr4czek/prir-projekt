#include <gtest/gtest.h>

#include "pipeline/border.hpp"

namespace {

// For the row abcdefgh (n = 8) the index -1 reads b and the index n reads g, so the edge pixel
// is not repeated. A one-pixel row has nothing to mirror, so every index reads pixel 0.
TEST(Reflect101, MapsOutsideIndicesToPixelsInside) {
    EXPECT_EQ(reflect101(-1, 8), 1);
    EXPECT_EQ(reflect101(8, 8), 6);
    EXPECT_EQ(reflect101(123, 1), 0);
}

}  // namespace
