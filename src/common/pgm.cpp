#include "pipeline/pgm.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>

namespace {

// Reads one whitespace-separated header token and skips comments, which run from '#' to the end
// of the line. Exactly one whitespace character after the token is consumed. That matters after
// the last header value, because the byte right behind that character is already the first pixel.
std::string read_token(std::istream& in) {
    std::string token;
    for (char c; in.get(c);) {
        if (c == '#') {
            in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        } else if (!std::isspace(static_cast<unsigned char>(c))) {
            token += c;
            continue;
        }
        if (!token.empty()) break;
    }
    return token;
}

// Returns -1 for a token that is not a whole number.
int to_number(const std::string& token) {
    int value = -1;
    const char* const end = token.data() + token.size();
    const auto result = std::from_chars(token.data(), end, value);
    return result.ec == std::errc() && result.ptr == end ? value : -1;
}

}  // namespace

Image<uint8_t> read_pgm(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + path.string());

    const std::string magic = read_token(in);
    const int width = to_number(read_token(in));
    const int height = to_number(read_token(in));
    const int max_value = to_number(read_token(in));
    if (magic != "P5" || width <= 0 || height <= 0 || max_value != 255)
        throw std::runtime_error(path.string() + " is not an 8-bit binary PGM file");

    Image<uint8_t> image(width, height);
    const auto size = static_cast<std::streamsize>(image.pixels.size());
    if (!in.read(reinterpret_cast<char*>(image.pixels.data()), size))
        throw std::runtime_error(path.string() + " ends before its last pixel");
    return image;
}

void write_pgm(const std::filesystem::path& path, ImageView<const uint8_t> image) {
    std::ofstream out(path, std::ios::binary);
    out << "P5\n" << image.width << ' ' << image.height << "\n255\n";
    // Rows are written one by one, because a view may keep padding after each row.
    for (int y = 0; y < image.height; ++y)
        out.write(reinterpret_cast<const char*>(image.row(y)), image.width);
    out.flush();
    if (!out) throw std::runtime_error("cannot write " + path.string());
}

std::vector<std::filesystem::path> list_frames(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> frames;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
        if (entry.is_regular_file() && entry.path().extension() == ".pgm") frames.push_back(entry.path());
    std::sort(frames.begin(), frames.end());
    return frames;
}
