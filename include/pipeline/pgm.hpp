#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "pipeline/image.hpp"

// PGM is the simplest lossless image format: a short text header ("P5", width, height, 255)
// followed by the raw gray8 bytes, row by row. Frames are kept in it so that every engine reads
// exactly the same bytes. Both functions throw std::runtime_error when they fail.
Image<uint8_t> read_pgm(const std::filesystem::path& path);
void write_pgm(const std::filesystem::path& path, ImageView<const uint8_t> image);

// Returns the .pgm files of a directory sorted by name, which is the order of the frames.
std::vector<std::filesystem::path> list_frames(const std::filesystem::path& directory);
