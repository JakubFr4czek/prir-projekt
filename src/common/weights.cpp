#include <cmath>
#include <stdexcept>
#include <vector>

#include "pipeline/params.hpp"

Params make_params(double sigma, int radius, uint32_t threshold) {
    if (radius < 0 || radius > kMaxRadius) throw std::invalid_argument("radius outside 0..15");
    if (threshold > 65535) throw std::invalid_argument("threshold outside 0..65535");

    Params p;
    p.radius = radius;
    p.threshold = threshold;

    const int r = radius;
    if (r == 0) {  // identity filter: diagnostic mode without blur
        p.q[0] = 256;
        return p;
    }
    if (!(sigma > 0)) throw std::invalid_argument("sigma must be > 0");

    // Gaussian bell exp(-i² / (2σ²)) for i = -r..r, normalized below by its sum.
    std::vector<double> g(2 * r + 1);
    double total = 0.0;
    for (int i = -r; i <= r; ++i) {
        g[i + r] = std::exp(-double(i * i) / (2.0 * sigma * sigma));
        total += g[i + r];
    }

    // Scales the weights to integers that sum to 256, going from the outermost weight to the
    // centre and carrying each rounding error to the next weight. The sum of 256 keeps an image
    // of constant brightness unchanged.
    double err = 0.0;
    uint32_t side_sum = 0;
    for (int i = 0; i < r; ++i) {
        const double a = 256.0 * g[i] / total + err;
        const double v = std::floor(a + 0.5);  // halves round up
        err = a - v;
        p.q[i] = p.q[2 * r - i] = static_cast<uint32_t>(v);
        side_sum += static_cast<uint32_t>(v);
    }
    p.q[r] = 256 - 2 * side_sum;  // the centre completes the sum to 256
    return p;
}
