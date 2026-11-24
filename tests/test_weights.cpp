#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "pipeline/params.hpp"

namespace {

std::vector<uint32_t> weights(double sigma, int radius) {
    const Params p = make_params(sigma, radius, 0);
    return {p.q.begin(), p.q.begin() + 2 * p.radius + 1};
}

// Every engine relies on these exact numbers, so each reference row is checked in full.
TEST(GaussWeights, MatchReferenceTable) {
    struct Row {
        double sigma;
        int radius;
        std::vector<uint32_t> q;
    };
    const Row table[] = {
        {1.0, 1, {70, 116, 70}},
        {1.0, 3, {1, 14, 62, 102, 62, 14, 1}},
        {2.0, 6, {1, 2, 7, 16, 31, 45, 52, 45, 31, 16, 7, 2, 1}},
        {3.0, 9, {0, 1, 3, 4, 9, 14, 20, 28, 32, 34, 32, 28, 20, 14, 9, 4, 3, 1, 0}},
    };
    for (const Row& row : table) {
        SCOPED_TRACE(testing::Message() << "sigma " << row.sigma << ", radius " << row.radius);
        EXPECT_EQ(weights(row.sigma, row.radius), row.q);
    }
}

// Rounding errors are carried between weights, so the sum of 256 and the symmetry are checked
// also for settings outside the reference table, including the largest radius.
TEST(GaussWeights, SumTo256AndAreSymmetric) {
    const std::pair<double, int> settings[] = {{1.0, 1}, {1.0, 3}, {2.0, 6},  {3.0, 9},
                                               {0.5, 1}, {0.8, 3}, {5.0, 15}, {100.0, 15}};
    for (const auto& [sigma, radius] : settings) {
        SCOPED_TRACE(testing::Message() << "sigma " << sigma << ", radius " << radius);
        const std::vector<uint32_t> q = weights(sigma, radius);
        uint32_t sum = 0;
        for (uint32_t w : q) sum += w;
        EXPECT_EQ(sum, 256u);
        for (std::size_t i = 0; i < q.size(); ++i)
            EXPECT_EQ(q[i], q[q.size() - 1 - i]) << "i = " << i;
    }
}

}  // namespace
