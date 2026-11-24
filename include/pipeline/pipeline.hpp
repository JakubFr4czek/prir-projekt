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
