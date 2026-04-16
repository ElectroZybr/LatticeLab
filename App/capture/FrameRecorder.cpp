#include "FrameRecorder.h"

#include <cstdlib>
#include <format>

#include "Engine/metrics/Profiler.h"

#ifdef _WIN32
#include <windows.h>
#ifdef NEAR
#undef NEAR
#endif
#ifdef FAR
#undef FAR
#endif
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif
#else
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
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

    static const char* toInputPixelFormat(bgfx::TextureFormat::Enum format) {
        switch (format) {
        case bgfx::TextureFormat::RGBA8:
            return "rgba";
        case bgfx::TextureFormat::BGRA8:
            return "bgra";
        case bgfx::TextureFormat::RGB8:
            return "rgb24";
        default:
            return "rgba";
        }
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
        const char* pathEnv = std::getenv("PATH");
        if (!pathEnv) {
            return {};
        }

        std::istringstream stream(pathEnv);
        std::string dir;

        while (std::getline(stream, dir, ':')) {
            auto candidate = std::filesystem::path(dir) / "ffmpeg";
            std::error_code ec;
            auto status = std::filesystem::status(candidate, ec);

            if (!ec && std::filesystem::is_regular_file(status) &&
                (status.permissions() & std::filesystem::perms::owner_exec) != std::filesystem::perms::none) {
                return candidate;
            }
        }

        return {};
#endif
    }
}

FrameRecorder::~FrameRecorder() { stop(); }

void FrameRecorder::start(const std::filesystem::path& outputPath, CaptureSettings settings) {
    outputPath_ = outputPath;
    settings_ = settings;
    ffmpegPath_ = findFfmpegExecutable();

    recording_ = true;
    savedFrameCount_ = 0;
    droppedFrameCount_ = 0;
    nextFrameIndex_ = 0;

    writerThread_ = std::thread(&FrameRecorder::writerLoop, this);
}

void FrameRecorder::stop() {
    {
        std::lock_guard lock(queueMutex_);
        recording_ = false;
    }
    cv_.notify_one();
    if (writerThread_.joinable()) {
        writerThread_.join();
    }
    closeEncoder();
}

bool FrameRecorder::isAvailable() { return !findFfmpegExecutable().empty(); }

bool FrameRecorder::isRecording() const { return recording_; }

bool FrameRecorder::submit(const CapturedFrame& frame) {
    PROFILE_SCOPE("Capture::encodeFrame");

    if (frame.empty()) {
        return false;
    }
    if (!recording_) {
        return false;
    }

    std::lock_guard lock(queueMutex_);
    frameQueue_.push(std::move(frame));
    cv_.notify_one();
    return true;
}

void FrameRecorder::writerLoop() {
    while (true) {
        CapturedFrame frame;
        {
            std::unique_lock lock(queueMutex_);
            cv_.wait(lock, [this] { return !frameQueue_.empty() || !recording_; });

            if (frameQueue_.empty()) {
                return; // recording_ == false и очередь пуста
            }

            frame = std::move(frameQueue_.front());
            frameQueue_.pop();
        }

        if (!writeFrame(frame)) {
            recording_ = false;
            return;
        }

        ++nextFrameIndex_;
        ++savedFrameCount_;
    }
}

bool FrameRecorder::writeFrame(const CapturedFrame& frame) {
    if (encoderProcess_ == nullptr || encoderStdinWrite_ == nullptr) {
        if (!openEncoder(frame)) {
            return false;
        }
    }
    else if (frame.width != frameWidth_ || frame.height != frameHeight_) {
        return false;
    }

#ifdef _WIN32
    DWORD bytesWritten = 0;
    const BOOL writeOk = WriteFile(static_cast<HANDLE>(encoderStdinWrite_), frame.pixels.data(), static_cast<DWORD>(frame.pixels.size()),
                                   &bytesWritten, nullptr);
    if (!writeOk || bytesWritten != frame.pixels.size()) {
        closeEncoder();
        return false;
    }
#else
    const ssize_t written = write(reinterpret_cast<intptr_t>(encoderStdinWrite_), frame.pixels.data(), frame.pixels.size());
    if (written != static_cast<ssize_t>(frame.pixels.size())) {
        closeEncoder();
        return false;
    }
#endif

    return true;
}

uint64_t FrameRecorder::savedFrameCount() const { return savedFrameCount_; }

uint64_t FrameRecorder::droppedFrameCount() const { return droppedFrameCount_; }

size_t FrameRecorder::pendingFrameCount() const {
    std::lock_guard lock(queueMutex_);
    return frameQueue_.size();
}

bool FrameRecorder::openEncoder(const CapturedFrame& frame) {
#ifdef _WIN32
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
            << " -pix_fmt " << toInputPixelFormat(frame.format) << " -s " << frameWidth_ << "x" << frameHeight_ << " -r " << settings_.fps
            << " -i -"
            << " -an"
            << " -c:v libx264"
            << " -preset " << toPresetArg(settings_.preset) << " -crf " << settings_.crf;

    if (is444(settings_.pixelFormat)) {
        command << " -profile:v high444";
    }

    if (frame.yflip && needsPad) {
        command << " -vf vflip,pad=ceil(iw/2)*2:ceil(ih/2)*2";
    }
    else if (frame.yflip) {
        command << " -vf vflip";
    }
    else if (needsPad) {
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
#else
    if (ffmpegPath_.empty() || !std::filesystem::exists(ffmpegPath_)) {
        return false;
    }

    frameWidth_ = frame.width;
    frameHeight_ = frame.height;

    const std::filesystem::path outputPath = std::filesystem::absolute(outputPath_);
    const bool needsPad = (frameWidth_ % 2 != 0) || (frameHeight_ % 2 != 0);

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        frameWidth_ = frameHeight_ = 0;
        return false;
    }

    std::string sizeArg = std::format("{}x{}", frameWidth_, frameHeight_);
    std::string fpsArg = std::to_string(settings_.fps);
    std::string crfArg = std::to_string(settings_.crf);
    std::string pixFmtArg = toPixelFormatArg(settings_.pixelFormat);
    std::string presetArg = toPresetArg(settings_.preset);
    std::string outputStr = outputPath.string();

    std::vector<std::string> args = {
        ffmpegPath_.string(),
        "-y",
        "-loglevel",
        "error",
        "-f",
        "rawvideo",
        "-pix_fmt",
        toInputPixelFormat(frame.format),
        "-s",
        sizeArg,
        "-r",
        fpsArg,
        "-i",
        "-",
        "-an",
        "-c:v",
        "libx264",
        "-preset",
        presetArg,
        "-crf",
        crfArg,
    };

    if (is444(settings_.pixelFormat)) {
        args.emplace_back("-profile:v");
        args.emplace_back("high444");
    }

    if (frame.yflip && needsPad) {
        args.emplace_back("-vf");
        args.emplace_back("vflip,pad=ceil(iw/2)*2:ceil(ih/2)*2");
    }
    else if (frame.yflip) {
        args.emplace_back("-vf");
        args.emplace_back("vflip");
    }
    else if (needsPad) {
        args.emplace_back("-vf");
        args.emplace_back("pad=ceil(iw/2)*2:ceil(ih/2)*2");
    }

    args.emplace_back("-pix_fmt");
    args.emplace_back(pixFmtArg);
    args.emplace_back(outputStr);

    std::vector<const char*> argv;
    argv.reserve(args.size() + 1);
    for (const auto& a : args) {
        argv.emplace_back(a.data());
    }
    argv.emplace_back(nullptr);

    const pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        frameWidth_ = frameHeight_ = 0;
        return false;
    }

    if (pid == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        int devNull = open("/dev/null", O_WRONLY);
        if (devNull >= 0) {
            dup2(devNull, STDOUT_FILENO);
            dup2(devNull, STDERR_FILENO);
            close(devNull);
        }

        execvp(argv[0], const_cast<char* const*>(argv.data()));
        _exit(1);
    }

    close(pipefd[0]);

    encoderProcess_ = reinterpret_cast<void*>(static_cast<intptr_t>(pid));
    encoderStdinWrite_ = reinterpret_cast<void*>(static_cast<intptr_t>(pipefd[1]));
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
#else
    if (encoderStdinWrite_ != nullptr) {
        close(static_cast<int>(reinterpret_cast<intptr_t>(encoderStdinWrite_)));
        encoderStdinWrite_ = nullptr;
    }
    if (encoderProcess_ != nullptr) {
        const pid_t pid = static_cast<pid_t>(reinterpret_cast<intptr_t>(encoderProcess_));
        waitpid(pid, nullptr, 0);
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
