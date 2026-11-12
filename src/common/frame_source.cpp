#include "pipeline/frame_source.hpp"

#include <cstddef>
#include <stdexcept>
#include <vector>

#include "pipeline/pgm.hpp"
#ifdef PIPELINE_HAVE_VIDEO
#include "video.hpp"
#endif

namespace {

// Serves the PGM files of a directory one by one, in the order of their names.
class PgmDirectorySource final : public FrameSource {
public:
    explicit PgmDirectorySource(const std::filesystem::path& directory) : frames_(list_frames(directory)) {
        if (frames_.empty()) throw std::runtime_error(directory.string() + " contains no PGM frames");
    }

    bool next(Image<uint8_t>& gray, RgbImage* rgb) override {
        if (next_ == frames_.size()) return false;
        gray = read_pgm(frames_[next_++]);
        if (rgb) {
            // A PGM frame has no color, so all three channels repeat the gray value.
            rgb->width = gray.width;
            rgb->height = gray.height;
            rgb->rgb.resize(gray.pixels.size() * 3);
            for (std::size_t i = 0; i < gray.pixels.size(); ++i)
                rgb->rgb[3 * i] = rgb->rgb[3 * i + 1] = rgb->rgb[3 * i + 2] = gray.pixels[i];
        }
        return true;
    }

private:
    std::vector<std::filesystem::path> frames_;
    std::size_t next_ = 0;
};

}  // namespace

std::unique_ptr<FrameSource> open_source(const std::filesystem::path& input) {
    if (std::filesystem::is_directory(input)) return std::make_unique<PgmDirectorySource>(input);
#ifdef PIPELINE_HAVE_VIDEO
    return open_video(input);
#else
    throw std::runtime_error(input.string() + " is not a directory of PGM frames, and reading video files "
                                              "needs a build with PIPELINE_WITH_VIDEO=ON");
#endif
}
