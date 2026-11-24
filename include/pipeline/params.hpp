#pragma once

#include <array>
#include <cstdint>

constexpr int kMaxRadius = 15;

// Filter settings shared by every engine. The struct holds only plain fields, so MPI can send it
// as a sequence of bytes.
struct Params {
    int radius = 0;
    uint32_t threshold = 0;
    std::array<uint32_t, 2 * kMaxRadius + 1> q{};  // only the first 2·radius + 1 weights are used
};

// Builds the integer Gaussian weights q, which sum to 256, for the given sigma and radius.
// Radius 0 gives the single weight [256], which means no blur. The weights are always made here,
// because computing exp() separately on the GPU or on different machines could give other numbers.
// Throws std::invalid_argument when radius is outside 0..15, threshold is above 65535 or, for a
// non-zero radius, sigma is not positive.
Params make_params(double sigma, int radius, uint32_t threshold);
