#include "FrameRecorder.h"

#include <cstdlib>

#include "Engine/metrics/Profiler.h"

#ifdef _WIN32
#include <windows.h>
#undef NEAR
#undef FAR
#endif

namespace {
    std::string quoteForCmd(const std::filesystem::path& path) { return "\"" + path.string() + "\""; }

    const char* toPresetArg(CaptureSettings::Preset preset) {
        switch (preset) {
        case CaptureSettings::Preset::Ultrafast:
            return "ultrafast";
        case CaptureSettings::Preset::Veryfast:
            return "veryfast";
        case CaptureSettings::Preset::Faster:
            return "faster";
        case CaptureSettings::Preset::Fast:
            return "fast";
        case CaptureSettings::Preset::Medium:
            return "medium";
        }
        return "veryfast";
    }

    const char* toPixelFormatArg(CaptureSettings::PixelFormat pixelFormat) {
        switch (pixelFormat) {
        case CaptureSettings::PixelFormat::Yuv420p:
            return "yuv420p";
        case CaptureSettings::PixelFormat::Yuv444p:
            return "yuv444p";
        }
        return "yuv444p";
    }

    bool is444(CaptureSettings::PixelFormat pixelFormat) { return pixelFormat == CaptureSettings::PixelFormat::Yuv444p; }

    std::filesystem::path tryWherePath() {
#ifdef _WIN32
        std::wstring buffer(MAX_PATH, L'\0');
        DWORD length = SearchPathW(nullptr, L"ffmpeg.exe", nullptr, static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);

        if (length == 0) {
            return {};
        }

        if (length >= buffer.size()) {
            buffer.resize(length + 1, L'\0');
            length = SearchPathW(nullptr, L"ffmpeg.exe", nullptr, static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);
            if (length == 0) {
                return {};
            }
        }

        buffer.resize(length);
        const std::filesystem::path path(buffer);
        return std::filesystem::exists(path) ? path : std::filesystem::path{};
#else
        return {};
#endif
    }
}

FrameRecorder::~FrameRecorder() { stop(); }

void FrameRecorder::start(const std::filesystem::path& outputPath, CaptureSettings settings) {
    std::lock_guard lock(mutex_);
    outputPath_ = outputPath;
    ffmpegPath_ = findFfmpegExecutable();
    settings_ = settings;
    nextFrameIndex_ = 0;
    savedFrameCount_ = 0;
    frameWidth_ = 0;
    frameHeight_ = 0;
    recording_ = true;
}

void FrameRecorder::stop() {
    std::lock_guard lock(mutex_);
    closeEncoder();
    recording_ = false;
}

bool FrameRecorder::isAvailable() { return !findFfmpegExecutable().empty(); }

bool FrameRecorder::isRecording() const {
    std::lock_guard lock(mutex_);
    return recording_;
}

bool FrameRecorder::submit(CapturedFrame frame) {
    PROFILE_SCOPE("Capture::encodeFrame");

    if (frame.empty()) {
        return false;
    }

    std::lock_guard lock(mutex_);
    if (!recording_) {
        return false;
    }

    if (encoderProcess_ == nullptr || encoderStdinWrite_ == nullptr) {
        if (!openEncoder(frame)) {
            recording_ = false;
            return false;
        }
    }
    else if (frame.width != frameWidth_ || frame.height != frameHeight_) {
        closeEncoder();
        recording_ = false;
        return false;
    }

#ifdef _WIN32
    DWORD bytesWritten = 0;
    const BOOL writeOk = WriteFile(static_cast<HANDLE>(encoderStdinWrite_), frame.rgba.data(), static_cast<DWORD>(frame.rgba.size()),
                                   &bytesWritten, nullptr);
    if (!writeOk || bytesWritten != frame.rgba.size()) {
        closeEncoder();
        recording_ = false;
        return false;
    }
#else
    closeEncoder();
    recording_ = false;
    return false;
#endif

    ++nextFrameIndex_;
    ++savedFrameCount_;
    return true;
}

uint64_t FrameRecorder::savedFrameCount() const {
    std::lock_guard lock(mutex_);
    return savedFrameCount_;
}

uint64_t FrameRecorder::droppedFrameCount() const { return 0; }

size_t FrameRecorder::pendingFrameCount() const { return 0; }

bool FrameRecorder::openEncoder(const CapturedFrame& frame) {
#ifndef _WIN32
    return false;
#else
    if (ffmpegPath_.empty() || !std::filesystem::exists(ffmpegPath_)) {
        return false;
    }

    frameWidth_ = frame.width;
    frameHeight_ = frame.height;

    const std::filesystem::path outputPath = std::filesystem::absolute(outputPath_);
    const bool needsPad = (frameWidth_ % 2 != 0) || (frameHeight_ % 2 != 0);

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE stdinRead = nullptr;
    HANDLE stdinWrite = nullptr;
    if (!CreatePipe(&stdinRead, &stdinWrite, &sa, 0)) {
        frameWidth_ = 0;
        frameHeight_ = 0;
        return false;
    }

    if (!SetHandleInformation(stdinWrite, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(stdinRead);
        CloseHandle(stdinWrite);
        frameWidth_ = 0;
        frameHeight_ = 0;
        return false;
    }

    HANDLE nullHandle = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (nullHandle == INVALID_HANDLE_VALUE) {
        CloseHandle(stdinRead);
        CloseHandle(stdinWrite);
        frameWidth_ = 0;
        frameHeight_ = 0;
        return false;
    }

    std::ostringstream command;
    command << quoteForCmd(ffmpegPath_) << " -y"
            << " -loglevel error"
            << " -f rawvideo"
            << " -pix_fmt rgba"
            << " -s " << frameWidth_ << "x" << frameHeight_ << " -r " << settings_.fps << " -i -"
            << " -an"
            << " -c:v libx264"
            << " -preset " << toPresetArg(settings_.preset) << " -crf " << settings_.crf;

    if (is444(settings_.pixelFormat)) {
        command << " -profile:v high444";
    }

    if (needsPad) {
        command << " -vf pad=ceil(iw/2)*2:ceil(ih/2)*2";
    }

    command << " -pix_fmt " << toPixelFormatArg(settings_.pixelFormat) << " " << quoteForCmd(outputPath);

    std::string commandLine = command.str();

    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdInput = stdinRead;
    startupInfo.hStdOutput = nullHandle;
    startupInfo.hStdError = nullHandle;

    PROCESS_INFORMATION processInfo{};
    const BOOL created =
        CreateProcessA(nullptr, commandLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo);

    CloseHandle(stdinRead);
    CloseHandle(nullHandle);

    if (!created) {
        CloseHandle(stdinWrite);
        frameWidth_ = 0;
        frameHeight_ = 0;
        return false;
    }

    CloseHandle(processInfo.hThread);
    encoderProcess_ = processInfo.hProcess;
    encoderStdinWrite_ = stdinWrite;
    return true;
#endif
}

void FrameRecorder::closeEncoder() {
#ifdef _WIN32
    if (encoderStdinWrite_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(encoderStdinWrite_));
        encoderStdinWrite_ = nullptr;
    }
    if (encoderProcess_ != nullptr) {
        WaitForSingleObject(static_cast<HANDLE>(encoderProcess_), 15000);
        CloseHandle(static_cast<HANDLE>(encoderProcess_));
        encoderProcess_ = nullptr;
    }
#endif
    frameWidth_ = 0;
    frameHeight_ = 0;
}

std::filesystem::path FrameRecorder::findFfmpegExecutable() {
    if (const std::filesystem::path wherePath = tryWherePath(); !wherePath.empty()) {
        return wherePath;
    }

    return {};
}
