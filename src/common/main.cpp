#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "pipeline/frame_source.hpp"
#include "pipeline/pgm.hpp"

namespace {

struct Options {
    std::filesystem::path input;
    std::filesystem::path dump_frames;
};

std::runtime_error usage_error(const std::string& problem) {
    return std::runtime_error(problem + "\nusage: pipeline --input <video file or directory of PGM frames> "
                                        "--dump-frames <directory>");
}

// Every option is followed by exactly one value.
Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; i += 2) {
        const std::string name = argv[i];
        if (i + 1 == argc) throw usage_error("missing value for " + name);
        if (name == "--input") {
            options.input = argv[i + 1];
        } else if (name == "--dump-frames") {
            options.dump_frames = argv[i + 1];
        } else {
            throw usage_error("unknown option " + name);
        }
    }
    if (options.input.empty() || options.dump_frames.empty())
        throw usage_error("--input and --dump-frames are required");
    return options;
}

// The number is zero-padded so that sorting the file names gives the order of the frames.
std::string frame_name(std::size_t index) {
    char name[32];
    std::snprintf(name, sizeof(name), "frame_%06zu.pgm", index);
    return name;
}

// Saves every frame of the source as a PGM file. The engines are compared on such files, because
// they hold the same bytes on every run and several MPI processes can read them independently.
// A directory that already holds frames is refused, so that one data set never mixes two films.
void dump_frames(FrameSource& source, const std::filesystem::path& directory) {
    std::filesystem::create_directories(directory);
    if (!list_frames(directory).empty())
        throw std::runtime_error(directory.string() + " already contains PGM frames; remove them or choose "
                                                      "another directory");
    Image<uint8_t> gray;
    std::size_t count = 0;
    while (source.next(gray, nullptr)) write_pgm(directory / frame_name(count++), gray.view());
    std::cout << "wrote " << count << " frames (" << gray.width << 'x' << gray.height << ") to "
              << directory.string() << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        const std::unique_ptr<FrameSource> source = open_source(options.input);
        dump_frames(*source, options.dump_frames);
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "pipeline: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
