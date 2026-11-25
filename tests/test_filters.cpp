#include <gtest/gtest.h>

#include <cstdint>
#include <random>
#include <vector>

#include "pipeline/pipeline.hpp"

namespace {

// Runs both passes of the Gaussian blur over the whole image.
Image<uint8_t> blur(const Params& p, GrayIn in, Image<uint16_t>* a_out = nullptr) {
    Image<uint16_t> A(in.width, in.height);
    Image<uint8_t> B(in.width, in.height);
    for (int y = 0; y < in.height; ++y) gauss_h_row(p, in, A.view(), y);
    for (int y = 0; y < in.height; ++y) gauss_v_row(p, A.view(), B.view(), y);
    if (a_out) *a_out = A;
    return B;
}

// The 2D filter computed directly, with the weight q[i] * q[j] for every neighbour. It is slow
// but obviously correct, so it checks that splitting the blur into two passes changes nothing.
// It shares reflect101 with the code under test, whose own tests cover the border rule.
Image<uint8_t> blur_direct_2d(const Params& p, GrayIn in) {
    const int r = p.radius;
    Image<uint8_t> B(in.width, in.height);
    for (int y = 0; y < in.height; ++y)
        for (int x = 0; x < in.width; ++x) {
            uint32_t sum = 0;
            for (int j = -r; j <= r; ++j)
                for (int i = -r; i <= r; ++i)
                    sum += p.q[i + r] * p.q[j + r] *
                           in.at(reflect101(x + i, in.width), reflect101(y + j, in.height));
            B.view().at(x, y) = static_cast<uint8_t>((sum + 32768u) >> 16);
        }
    return B;
}

Image<uint8_t> random_image(int w, int h, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> value(0, 255);
    Image<uint8_t> img(w, h);
    for (uint8_t& px : img.pixels) px = static_cast<uint8_t>(value(rng));
    return img;
}

// One pixel of 255 in the middle of a 5×5 image, blurred with the weights [70, 116, 70], spreads
// its brightness over the neighbours while the total stays almost the same.
TEST(GaussBlur, ImpulseGivesExpectedImage) {
    Image<uint8_t> in(5, 5);
    in.view().at(2, 2) = 255;
    const Params p = make_params(1.0, 1, 0);

    Image<uint16_t> A;
    const Image<uint8_t> B = blur(p, in.view(), &A);

    const std::vector<uint16_t> middle_row_a(A.pixels.begin() + 2 * 5, A.pixels.begin() + 3 * 5);
    EXPECT_EQ(middle_row_a, (std::vector<uint16_t>{0, 17850, 29580, 17850, 0}));

    const std::vector<uint8_t> expected = {
        0, 0,  0,  0,  0,
        0, 19, 32, 19, 0,
        0, 32, 52, 32, 0,
        0, 19, 32, 19, 0,
        0, 0,  0,  0,  0,
    };
    EXPECT_EQ(B.pixels, expected);
}

// The weights sum to 256, so an image of one brightness must come out unchanged.
TEST(GaussBlur, ConstantImageStaysUnchanged) {
    for (const Params& p : {make_params(1.0, 1, 0), make_params(1.0, 3, 0), make_params(2.0, 6, 0),
                            make_params(3.0, 9, 0), make_params(1.0, 0, 0)}) {
        for (int value = 0; value <= 255; ++value) {
            Image<uint8_t> in(7, 5);
            for (uint8_t& px : in.pixels) px = static_cast<uint8_t>(value);
            const Image<uint8_t> B = blur(p, in.view());
            ASSERT_EQ(B.pixels, in.pixels) << "radius " << p.radius << ", value " << value;
        }
    }
}

// Two 1D passes must give exactly the direct 2D filter, including at the borders and when the
// radius is larger than the image.
TEST(GaussBlur, SeparablePassesMatchDirect2D) {
    const Params params[] = {make_params(1.0, 1, 0), make_params(1.0, 3, 0), make_params(2.0, 6, 0),
                             make_params(3.0, 9, 0), make_params(1.0, 0, 0)};
    const int sizes[][2] = {{1, 1}, {1, 9}, {9, 1}, {2, 3}, {17, 11}, {40, 31}};
    unsigned seed = 1;
    for (const Params& p : params)
        for (const auto& size : sizes) {
            const Image<uint8_t> in = random_image(size[0], size[1], seed++);
            ASSERT_EQ(blur(p, in.view()).pixels, blur_direct_2d(p, in.view()).pixels)
                << "radius " << p.radius << ", image " << size[0] << "x" << size[1];
        }
}

// D of the 3×3 neighbourhood given row by row, with the centre at n[1][1].
uint32_t sobel_of(const int (&n)[3][3]) {
    return sobel_d([&](int dx, int dy) { return n[1 + dy][1 + dx]; });
}

// D at (x, y) of a whole image, with neighbours outside it read through reflect101.
uint32_t sobel_at(GrayIn B, int x, int y) {
    return sobel_d([&](int dx, int dy) -> int {
        return B.at(reflect101(x + dx, B.width), reflect101(y + dy, B.height));
    });
}

TEST(Sobel, UniformNeighbourhoodGivesZero) {
    EXPECT_EQ(sobel_of({{20, 20, 20}, {20, 20, 20}, {20, 20, 20}}), 0u);
}

// gx = (-20 + 200) + 2 * (-20 + 200) + (-20 + 200) = 720 and gy = 0.
TEST(Sobel, VerticalEdgeGivesHorizontalChange) {
    EXPECT_EQ(sobel_of({{20, 20, 200}, {20, 20, 200}, {20, 20, 200}}), 518400u);
}

// The edge runs diagonally, so the change is equal in both directions: gx = gy = 540.
TEST(Sobel, DiagonalEdgeGivesBothChanges) {
    EXPECT_EQ(sobel_of({{20, 20, 20}, {20, 20, 200}, {20, 200, 200}}), 583200u);
}

// Column 0 is bright and the rest is dark. At x = 0 reflect101 reads column 1 for the missing
// column -1, so both sides are dark and D = 0; repeating the edge pixel would give D = 518,400.
// At x = 1 the same edge is seen from inside the image, which shows that it is really there.
TEST(Sobel, ReadsPixelsOutsideTheImageThroughReflect101) {
    Image<uint8_t> B(4, 3);
    for (int y = 0; y < B.height; ++y)
        for (int x = 0; x < B.width; ++x) B.view().at(x, y) = x == 0 ? 200 : 20;

    for (int y = 0; y < B.height; ++y) {
        EXPECT_EQ(sobel_at(B.view(), 0, y), 0u) << "y = " << y;
        EXPECT_EQ(sobel_at(B.view(), 1, y), 518400u) << "y = " << y;
    }
}

}  // namespace
