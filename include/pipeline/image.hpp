#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

// A frame is a grid of pixels stored row by row; input frames are gray8, which means one uint8_t
// per pixel, from 0 (black) to 255 (white). ImageView points at pixels it does not own. Its
// `stride` is the distance between the starts of two consecutive rows in elements of T and can
// exceed `width`, for example in a window into a bigger image, so a pixel is at data[y * stride + x].
template <class T>
struct ImageView {
    T* data = nullptr;
    int width = 0;
    int height = 0;
    std::ptrdiff_t stride = 0;

    T* row(int y) const { return data + y * stride; }
    T& at(int x, int y) const { return row(y)[x]; }

    // Lets a writable view be passed where a function only reads.
    template <class U = T, std::enable_if_t<!std::is_const_v<U>, int> = 0>
    operator ImageView<const U>() const { return {data, width, height, stride}; }
};

// Owns its pixels and stores them without padding, so its stride equals its width.
template <class T>
struct Image {
    std::vector<T> pixels;
    int width = 0;
    int height = 0;

    Image() = default;
    Image(int w, int h) : pixels(std::size_t(w) * std::size_t(h)), width(w), height(h) {}

    ImageView<T> view() { return {pixels.data(), width, height, width}; }
    ImageView<const T> view() const { return {pixels.data(), width, height, width}; }
};

// Color frame that the overlay is drawn on: three bytes (R, G, B) per pixel, with the rows
// stored one after another without padding.
struct RgbImage {
    std::vector<uint8_t> rgb;
    int width = 0;
    int height = 0;
};
