#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "pipeline/frame_source.hpp"
#include "pipeline/pgm.hpp"

namespace {

namespace fs = std::filesystem;

// Each test works in its own temporary directory, which is removed afterwards.
class PgmTest : public testing::Test {
protected:
    void SetUp() override {
        dir_ = fs::temp_directory_path() / ("pipeline_pgm_test_" + std::to_string(std::random_device{}()));
        fs::create_directories(dir_);
    }
    void TearDown() override { fs::remove_all(dir_); }

    fs::path dir_;
};

// The first pixels are bytes that also have a meaning in the header: newline, space, '#' and tab.
// A reader that skipped more than the single whitespace character after the header would lose them.
Image<uint8_t> make_image(int width, int height) {
    Image<uint8_t> image(width, height);
    const uint8_t header_like[] = {'\n', ' ', '#', '\t'};
    for (std::size_t i = 0; i < image.pixels.size(); ++i)
        image.pixels[i] = i < 4 ? header_like[i] : static_cast<uint8_t>(i * 31);
    return image;
}

TEST_F(PgmTest, WriteThenReadReturnsTheSameBytes) {
    const Image<uint8_t> image = make_image(7, 5);

    write_pgm(dir_ / "frame.pgm", image.view());
    const Image<uint8_t> loaded = read_pgm(dir_ / "frame.pgm");

    EXPECT_EQ(loaded.width, 7);
    EXPECT_EQ(loaded.height, 5);
    EXPECT_EQ(loaded.pixels, image.pixels);
}

TEST_F(PgmTest, WriteLeavesOutPaddingWhenStrideIsLargerThanWidth) {
    // The same pixels as in `image`, but every row is followed by three bytes of padding.
    const Image<uint8_t> image = make_image(7, 5);
    const std::ptrdiff_t stride = 10;
    std::vector<uint8_t> padded(static_cast<std::size_t>(stride) * 5, 0xEE);
    for (int y = 0; y < 5; ++y) std::copy_n(image.view().row(y), 7, padded.begin() + y * stride);

    write_pgm(dir_ / "frame.pgm", ImageView<const uint8_t>{padded.data(), 7, 5, stride});
    const Image<uint8_t> loaded = read_pgm(dir_ / "frame.pgm");

    EXPECT_EQ(loaded.width, 7);
    EXPECT_EQ(loaded.height, 5);
    EXPECT_EQ(loaded.pixels, image.pixels);
}

TEST_F(PgmTest, ReadSkipsCommentsInTheHeader) {
    std::ofstream(dir_ / "frame.pgm", std::ios::binary)
        << "P5\n# written by hand\n2 2 # width and height\n255\n"
        << "\n #x";

    const Image<uint8_t> loaded = read_pgm(dir_ / "frame.pgm");

    EXPECT_EQ(loaded.width, 2);
    EXPECT_EQ(loaded.height, 2);
    EXPECT_EQ(loaded.pixels, (std::vector<uint8_t>{'\n', ' ', '#', 'x'}));
}

TEST_F(PgmTest, ReadRejectsFilesThatAreNotCompleteGray8Images) {
    std::ofstream(dir_ / "color.ppm", std::ios::binary) << "P6\n1 1\n255\nabc";
    std::ofstream(dir_ / "deep.pgm", std::ios::binary) << "P5\n1 1\n65535\nab";
    std::ofstream(dir_ / "short.pgm", std::ios::binary) << "P5\n2 2\n255\nabc";

    EXPECT_THROW(read_pgm(dir_ / "color.ppm"), std::runtime_error);
    EXPECT_THROW(read_pgm(dir_ / "deep.pgm"), std::runtime_error);
    EXPECT_THROW(read_pgm(dir_ / "short.pgm"), std::runtime_error);
    EXPECT_THROW(read_pgm(dir_ / "missing.pgm"), std::runtime_error);
}

TEST_F(PgmTest, OpenSourceReadsADirectoryInNameOrder) {
    // Frames are written out of order, next to a file that is not a frame, as in a data set
    // that is stored together with its manifest.
    for (const int index : {2, 0, 1}) {
        Image<uint8_t> frame(4, 2);
        std::fill(frame.pixels.begin(), frame.pixels.end(), static_cast<uint8_t>(index));
        write_pgm(dir_ / ("frame_00000" + std::to_string(index) + ".pgm"), frame.view());
    }
    std::ofstream(dir_ / "manifest.json") << "{}";

    const std::unique_ptr<FrameSource> source = open_source(dir_);
    Image<uint8_t> gray;
    RgbImage rgb;
    for (int index = 0; index < 3; ++index) {
        ASSERT_TRUE(source->next(gray, &rgb));
        EXPECT_EQ(gray.pixels, std::vector<uint8_t>(8, static_cast<uint8_t>(index)));
        EXPECT_EQ(rgb.rgb, std::vector<uint8_t>(24, static_cast<uint8_t>(index)));
    }
    EXPECT_FALSE(source->next(gray, &rgb));
}

TEST_F(PgmTest, OpenSourceRejectsADirectoryWithoutFrames) {
    EXPECT_THROW(open_source(dir_), std::runtime_error);
}

}  // namespace
