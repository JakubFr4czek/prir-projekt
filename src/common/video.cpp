#include "video.hpp"

#include <cstddef>
#include <new>
#include <stdexcept>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace {

// FFmpeg reports a failure as a negative return code. This turns it into an exception that
// carries FFmpeg's description of the error, and passes other results through.
int check(int result, const std::string& what) {
    if (result < 0) {
        char reason[AV_ERROR_MAX_STRING_SIZE] = {};
        av_strerror(result, reason, sizeof(reason));
        throw std::runtime_error(what + ": " + reason);
    }
    return result;
}

// Deleters that let std::unique_ptr own FFmpeg objects, so they are released on every exit path.
struct FormatDeleter {
    void operator()(AVFormatContext* format) const { avformat_close_input(&format); }
};
struct DecoderDeleter {
    void operator()(AVCodecContext* decoder) const { avcodec_free_context(&decoder); }
};
struct PacketDeleter {
    void operator()(AVPacket* packet) const { av_packet_free(&packet); }
};
struct FrameDeleter {
    void operator()(AVFrame* frame) const { av_frame_free(&frame); }
};
struct ScalerDeleter {
    void operator()(SwsContext* scaler) const { sws_freeContext(scaler); }
};
using ScalerPtr = std::unique_ptr<SwsContext, ScalerDeleter>;

// These flags make swscale use its reference C code instead of CPU-specific optimizations, whose
// rounding can differ slightly. The conversion then gives the same bytes regardless of the CPU.
constexpr int scaler_flags = SWS_ACCURATE_RND | SWS_BITEXACT;

class VideoSource final : public FrameSource {
public:
    explicit VideoSource(const std::filesystem::path& file) {
        const std::string name = file.string();

        AVFormatContext* format = nullptr;
        check(avformat_open_input(&format, name.c_str(), nullptr, nullptr), "cannot open " + name);
        format_.reset(format);
        check(avformat_find_stream_info(format, nullptr), "cannot read the streams of " + name);
        const AVCodec* codec = nullptr;
        stream_ = check(av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0),
                        "cannot find a video stream to decode in " + name);

        decoder_.reset(avcodec_alloc_context3(codec));
        packet_.reset(av_packet_alloc());
        frame_.reset(av_frame_alloc());
        if (!decoder_ || !packet_ || !frame_) throw std::bad_alloc();
        check(avcodec_parameters_to_context(decoder_.get(), format->streams[stream_]->codecpar),
              "cannot configure the decoder of " + name);
        check(avcodec_open2(decoder_.get(), codec, nullptr), "cannot open the decoder of " + name);
    }

    bool next(Image<uint8_t>& gray, RgbImage* rgb) override {
        if (!decode()) return false;

        // swscale converts the decoded picture, which is usually YUV, to gray8 and, when the overlay
        // needs it, to RGB. Every engine gets its input from this one conversion.
        const int width = frame_->width;
        const int height = frame_->height;
        if (gray.width != width || gray.height != height) gray = Image<uint8_t>(width, height);
        convert(to_gray_, AV_PIX_FMT_GRAY8, gray.pixels.data(), width);
        if (rgb) {
            rgb->width = width;
            rgb->height = height;
            rgb->rgb.resize(std::size_t(width) * std::size_t(height) * 3);
            convert(to_rgb_, AV_PIX_FMT_RGB24, rgb->rgb.data(), width * 3);
        }
        return true;
    }

private:
    // The decoder is fed packets until it returns a picture. It keeps several pictures buffered,
    // so at the end of the file an empty packet tells it to hand out the remaining ones.
    bool decode() {
        for (;;) {
            const int received = avcodec_receive_frame(decoder_.get(), frame_.get());
            if (received == 0) return true;
            if (received == AVERROR_EOF) return false;
            if (received != AVERROR(EAGAIN)) check(received, "cannot decode the video");

            // The decoder needs more input: the next packet of the video stream, or the end of the file.
            int read = 0;
            do {
                av_packet_unref(packet_.get());
                read = av_read_frame(format_.get(), packet_.get());
            } while (read == 0 && packet_->stream_index != stream_);
            if (read != AVERROR_EOF) check(read, "cannot read the video file");
            check(avcodec_send_packet(decoder_.get(), read == 0 ? packet_.get() : nullptr),
                  "cannot decode the video");
            av_packet_unref(packet_.get());
        }
    }

    // Converts the decoded frame into a buffer of the caller whose rows are `stride` bytes apart.
    // The scaler is created for the first frame and reused while the frame size and format stay the same.
    void convert(ScalerPtr& scaler, AVPixelFormat format, uint8_t* out, int stride) {
        const AVFrame& in = *frame_;
        const auto in_format = static_cast<AVPixelFormat>(in.format);
        scaler.reset(sws_getCachedContext(scaler.release(), in.width, in.height, in_format,
                                          in.width, in.height, format, scaler_flags,
                                          nullptr, nullptr, nullptr));
        if (!scaler) throw std::runtime_error("cannot convert the pixel format of the video");
        uint8_t* const planes[4] = {out};
        const int strides[4] = {stride};
        check(sws_scale(scaler.get(), in.data, in.linesize, 0, in.height, planes, strides),
              "cannot convert a video frame");
    }

    std::unique_ptr<AVFormatContext, FormatDeleter> format_;
    std::unique_ptr<AVCodecContext, DecoderDeleter> decoder_;
    std::unique_ptr<AVPacket, PacketDeleter> packet_;
    std::unique_ptr<AVFrame, FrameDeleter> frame_;
    ScalerPtr to_gray_;
    ScalerPtr to_rgb_;
    int stream_ = -1;
};

}  // namespace

std::unique_ptr<FrameSource> open_video(const std::filesystem::path& file) {
    return std::make_unique<VideoSource>(file);
}
