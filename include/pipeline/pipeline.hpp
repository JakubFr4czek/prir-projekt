#pragma once

#include <cstdint>

#include "pipeline/border.hpp"
#include "pipeline/image.hpp"
#include "pipeline/params.hpp"

using GrayIn = ImageView<const uint8_t>;
using GrayOut = ImageView<uint8_t>;

// The Gaussian blur is separable, so it runs as two 1D passes, first horizontal, then vertical.
// Each function computes one row y, which lets the OpenMP engine split rows between threads.
//
// The horizontal pass writes A = sum of q[i] * I(x + i, y) without dividing, at most
// 255 * 256 = 65,280. Rounding A here would change the algorithm, so A stays uint16_t.
void gauss_h_row(const Params& p, GrayIn I, ImageView<uint16_t> A, int y);

// The vertical pass computes V = sum of q[j] * A(x, y + j) and stores B = (V + 32768) >> 16,
// which divides by 65,536 = 256² and rounds halves up. This is the only rounding of the blur.
void gauss_v_row(const Params& p, ImageView<const uint16_t> A, GrayOut B, int y);

// The Sobel filter measures how sharply brightness changes around a pixel of the blurred image B.
// gx > 0 means it is brighter to the right, gy > 0 that it is brighter below. Only the strength
// of the change matters, so the result is D = gx² + gy², at most 2 * 1020² = 2,080,800.
// `a(dx, dy)` returns the pixel at offset (dx, dy), each in -1..1, as an int. The caller applies
// the border rule, so the same code runs on the CPU and the GPU.
template <class Sample>
PIPELINE_FN inline uint32_t sobel_d(Sample a) {
    const int gx = -a(-1, -1) + a(1, -1) - 2 * a(-1, 0) + 2 * a(1, 0) - a(-1, 1) + a(1, 1);
    const int gy = -a(-1, -1) - 2 * a(0, -1) - a(1, -1) + a(-1, 1) + 2 * a(0, 1) + a(1, 1);
    return static_cast<uint32_t>(gx * gx + gy * gy);
}
