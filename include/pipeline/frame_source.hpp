#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

#include "pipeline/image.hpp"

// A sequence of frames. It hides whether the frames come from a video file or from a directory
// of PGM images, so the rest of the program handles both in the same way.
class FrameSource {
public:
    virtual ~FrameSource() = default;

    // Returns false when there are no more frames. `rgb` may be null; it is filled only when
    // the caller needs the color picture, which is the case when the overlay is produced.
    virtual bool next(Image<uint8_t>& gray, RgbImage* rgb) = 0;
};

// Opens a directory of PGM frames or a video file, depending on what `input` points to.
std::unique_ptr<FrameSource> open_source(const std::filesystem::path& input);
