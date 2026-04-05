#include "FrameRecorder.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
void writeLE16(std::ostream& out, uint16_t value) {
    const std::array<char, 2> bytes{
        static_cast<char>(value & 0xFF),
        static_cast<char>((value >> 8) & 0xFF)
    };
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

void writeLE32(std::ostream& out, uint32_t value) {
    const std::array<char, 4> bytes{
        static_cast<char>(value & 0xFF),
        static_cast<char>((value >> 8) & 0xFF),
        static_cast<char>((value >> 16) & 0xFF),
        static_cast<char>((value >> 24) & 0xFF)
    };
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

void writeLE32Signed(std::ostream& out, int32_t value) {
    writeLE32(out, static_cast<uint32_t>(value));
}
}

FrameRecorder::~FrameRecorder() {
    stop();
}

void FrameRecorder::start(const std::filesystem::path& sessionDir) {
    std::lock_guard lock(mutex_);
    sessionDir_ = sessionDir;
    nextFrameIndex_ = 0;
    savedFrameCount_ = 0;
    recording_ = true;
}

void FrameRecorder::stop() {
    std::lock_guard lock(mutex_);
    recording_ = false;
}

bool FrameRecorder::isRecording() const {
    std::lock_guard lock(mutex_);
    return recording_;
}

bool FrameRecorder::submit(CapturedFrame frame) {
    if (frame.empty()) {
        return false;
    }

    uint64_t frameIndex = 0;
    std::filesystem::path sessionDir;

    {
        std::lock_guard lock(mutex_);
        if (!recording_) {
            return false;
        }
        frameIndex = nextFrameIndex_++;
        sessionDir = sessionDir_;
    }

    const bool saved = writeBmp(makeFramePath(sessionDir, frameIndex), frame);

    if (saved) {
        std::lock_guard lock(mutex_);
        ++savedFrameCount_;
    }

    return saved;
}

uint64_t FrameRecorder::savedFrameCount() const {
    std::lock_guard lock(mutex_);
    return savedFrameCount_;
}

uint64_t FrameRecorder::droppedFrameCount() const {
    return 0;
}

size_t FrameRecorder::pendingFrameCount() const {
    return 0;
}

std::filesystem::path FrameRecorder::makeFramePath(const std::filesystem::path& sessionDir, uint64_t frameIndex) {
    std::ostringstream fileName;
    fileName << "frame_" << std::setfill('0') << std::setw(6) << frameIndex << ".bmp";
    return sessionDir / fileName.str();
}

bool FrameRecorder::writeBmp(const std::filesystem::path& path, const CapturedFrame& frame) {
    if (frame.empty()) {
        return false;
    }

    constexpr uint32_t kFileHeaderSize = 14;
    constexpr uint32_t kInfoHeaderSize = 40;
    constexpr uint32_t kPixelOffset = kFileHeaderSize + kInfoHeaderSize;
    constexpr uint32_t kBitsPerPixel = 32;
    const uint32_t pixelDataSize = frame.width * frame.height * 4;
    const uint32_t fileSize = kPixelOffset + pixelDataSize;

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }

    out.put('B');
    out.put('M');
    writeLE32(out, fileSize);
    writeLE16(out, 0);
    writeLE16(out, 0);
    writeLE32(out, kPixelOffset);

    writeLE32(out, kInfoHeaderSize);
    writeLE32(out, frame.width);
    writeLE32Signed(out, -static_cast<int32_t>(frame.height));
    writeLE16(out, 1);
    writeLE16(out, static_cast<uint16_t>(kBitsPerPixel));
    writeLE32(out, 0);
    writeLE32(out, pixelDataSize);
    writeLE32Signed(out, 2835);
    writeLE32Signed(out, 2835);
    writeLE32(out, 0);
    writeLE32(out, 0);

    thread_local std::vector<uint8_t> bgra;
    bgra.resize(frame.rgba.size());
    for (size_t i = 0; i < frame.rgba.size(); i += 4) {
        bgra[i + 0] = frame.rgba[i + 2];
        bgra[i + 1] = frame.rgba[i + 1];
        bgra[i + 2] = frame.rgba[i + 0];
        bgra[i + 3] = frame.rgba[i + 3];
    }

    out.write(reinterpret_cast<const char*>(bgra.data()), static_cast<std::streamsize>(bgra.size()));
    return static_cast<bool>(out);
}
