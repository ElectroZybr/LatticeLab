#include "FrameRecorder.h"

#include <cstdlib>
#include <sstream>
#include <vector>

#include "Engine/metrics/Profiler.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
std::string quoteForCmd(const std::filesystem::path& path) {
    return "\"" + path.string() + "\"";
}

std::filesystem::path tryEnvPath() {
    if (const char* envPath = std::getenv("FFMPEG_PATH")) {
        const std::filesystem::path path(envPath);
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    return {};
}

std::filesystem::path tryWherePath() {
    FILE* pipe = _popen("where ffmpeg 2>nul", "r");
    if (pipe == nullptr) {
        return {};
    }

    char buffer[1024];
    std::string firstLine;
    if (fgets(buffer, static_cast<int>(sizeof(buffer)), pipe) != nullptr) {
        firstLine = buffer;
    }
    _pclose(pipe);

    while (!firstLine.empty() && (firstLine.back() == '\n' || firstLine.back() == '\r')) {
        firstLine.pop_back();
    }

    if (firstLine.empty()) {
        return {};
    }

    const std::filesystem::path path(firstLine);
    return std::filesystem::exists(path) ? path : std::filesystem::path{};
}
}

FrameRecorder::~FrameRecorder() {
    stop();
}

void FrameRecorder::start(const std::filesystem::path& sessionDir) {
    std::lock_guard lock(mutex_);
    sessionDir_ = sessionDir;
    ffmpegPath_ = findFfmpegExecutable();
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
    } else if (frame.width != frameWidth_ || frame.height != frameHeight_) {
        closeEncoder();
        recording_ = false;
        return false;
    }

#ifdef _WIN32
    DWORD bytesWritten = 0;
    const BOOL writeOk = WriteFile(
        static_cast<HANDLE>(encoderStdinWrite_),
        frame.rgba.data(),
        static_cast<DWORD>(frame.rgba.size()),
        &bytesWritten,
        nullptr
    );
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

uint64_t FrameRecorder::droppedFrameCount() const {
    return 0;
}

size_t FrameRecorder::pendingFrameCount() const {
    return 0;
}

bool FrameRecorder::openEncoder(const CapturedFrame& frame) {
#ifndef _WIN32
    return false;
#else
    if (ffmpegPath_.empty() || !std::filesystem::exists(ffmpegPath_)) {
        return false;
    }

    frameWidth_ = frame.width;
    frameHeight_ = frame.height;

    const std::filesystem::path outputPath = std::filesystem::absolute(makeVideoPath(sessionDir_));
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

    HANDLE nullHandle = CreateFileW(
        L"NUL",
        GENERIC_WRITE,
        FILE_SHARE_READ,
        &sa,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (nullHandle == INVALID_HANDLE_VALUE) {
        CloseHandle(stdinRead);
        CloseHandle(stdinWrite);
        frameWidth_ = 0;
        frameHeight_ = 0;
        return false;
    }

    std::ostringstream command;
    command
        << quoteForCmd(ffmpegPath_)
        << " -y"
        << " -loglevel error"
        << " -f rawvideo"
        << " -pix_fmt rgba"
        << " -s " << frameWidth_ << "x" << frameHeight_
        << " -r 60"
        << " -i -"
        << " -an"
        << " -c:v libx264"
        << " -preset ultrafast"
        << " -crf 18";

    if (needsPad) {
        command << " -vf pad=ceil(iw/2)*2:ceil(ih/2)*2";
    }

    command
        << " -pix_fmt yuv420p"
        << " " << quoteForCmd(outputPath);

    std::string commandLine = command.str();

    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdInput = stdinRead;
    startupInfo.hStdOutput = nullHandle;
    startupInfo.hStdError = nullHandle;

    PROCESS_INFORMATION processInfo{};
    const BOOL created = CreateProcessA(
        nullptr,
        commandLine.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo
    );

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

std::filesystem::path FrameRecorder::makeVideoPath(const std::filesystem::path& sessionDir) {
    return sessionDir / "capture.mp4";
}

std::filesystem::path FrameRecorder::findFfmpegExecutable() {
    const std::vector<std::filesystem::path> fallbackPaths{
        std::filesystem::current_path() / "assets" / "ffmpeg" / "ffmpeg.exe",
        std::filesystem::current_path() / "ffmpeg.exe",
        std::filesystem::current_path() / "tools" / "ffmpeg" / "bin" / "ffmpeg.exe"
    };

    for (const auto& path : fallbackPaths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    if (const std::filesystem::path envPath = tryEnvPath(); !envPath.empty()) {
        return envPath;
    }

    if (const std::filesystem::path wherePath = tryWherePath(); !wherePath.empty()) {
        return wherePath;
    }

    return {};
}
