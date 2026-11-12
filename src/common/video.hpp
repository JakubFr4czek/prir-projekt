#pragma once

#include <filesystem>
#include <memory>

#include "pipeline/frame_source.hpp"

// Opens a video file as a frame source that decodes it with the FFmpeg libraries, inside this process.
std::unique_ptr<FrameSource> open_video(const std::filesystem::path& file);
