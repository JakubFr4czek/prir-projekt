// Reference engine: readable code without optimizations, which the other engines must match.
#include "pipeline/pipeline.hpp"

void gauss_h_row(const Params& p, GrayIn I, ImageView<uint16_t> A, int y) {
    const uint8_t* src = I.row(y);
    uint16_t* dst = A.row(y);
    for (int x = 0; x < I.width; ++x) {
        uint32_t sum = 0;
        for (int i = -p.radius; i <= p.radius; ++i)
            sum += p.q[i + p.radius] * src[reflect101(x + i, I.width)];
        dst[x] = static_cast<uint16_t>(sum);  // at most 255 * 256 = 65,280
    }
}

void gauss_v_row(const Params& p, ImageView<const uint16_t> A, GrayOut B, int y) {
    uint8_t* dst = B.row(y);
    for (int x = 0; x < A.width; ++x) {
        uint32_t sum = 0;
        for (int j = -p.radius; j <= p.radius; ++j)
            sum += p.q[j + p.radius] * A.at(x, reflect101(y + j, A.height));
        dst[x] = static_cast<uint8_t>((sum + 32768u) >> 16);  // the only rounding
    }
}
