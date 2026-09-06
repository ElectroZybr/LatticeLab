#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <Lattice/Tools/Logger.hpp>

namespace {

std::string timestampForLogLine() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};

#if defined(_WIN32)
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif

    std::ostringstream out;
    out << std::put_time(&localTime, "%H:%M:%S");
    return out.str();
}


constexpr uint8_t kFinalBit  = 0x08;
constexpr uint8_t kPinnedBit = 0x10;

constexpr uint8_t packLine(Level level, size_t depth, bool isScopeFinal) noexcept {
    const uint8_t packedDepth = static_cast<uint8_t>(depth < 7 ? depth : 7);
    return static_cast<uint8_t>(
        (static_cast<uint8_t>(level) & 0x07) |
        (isScopeFinal ? kFinalBit : 0) |
        (packedDepth << 5)
    );
}

constexpr Level unpackLevel(uint8_t packed) noexcept {
    return static_cast<Level>(packed & 0x07);
}

constexpr bool unpackFinal(uint8_t packed) noexcept {
    return (packed & kFinalBit) != 0;
}

constexpr bool unpackPinned(uint8_t packed) noexcept {
    return (packed & kPinnedBit) != 0;
}

constexpr size_t unpackDepth(uint8_t packed) noexcept {
    return packed >> 5;
}

}

void LogSystem::write(Level level, const Text& text) {
    std::lock_guard lock(mutex_);

    if (!file_.is_open())
        return;

    const auto& style = LogStyle::get(level);

    file_ << timestampForLogLine()
          << ' '
          << std::format("[{}] {}", style.label, text.plain())
          << '\n';
}

void LoggerImpl::print(Level level, const Text& text, bool isScopeFinal) {
    LogSystem::write(level, text);

    if (scopeCount_ != 0) {
        const auto& scope = scopes_[scopeCount_ - 1];

        if (level == Level::Warning ||
            (LogModes::isError(level) && !hasMode(scope.mode, LogMode::SuppressError)))
        {
            for (uint8_t i = 0; i < scopeCount_; ++i)
                scopes_[i].hadProblem = true;
        }

        if (!isScopeFinal &&
            level != Level::Action &&
            !LogModes::shouldKeep({
                .mode = scope.mode,
                .level = level,
                .success = true,
                .depth = scopeCount_,
                .maxDepth = scope.maxDepth,
                .hadProblem = scope.hadProblem
            }))
        {
            return;
        }

        lines_.push_back(packLine(level, scopeCount_, isScopeFinal));
    }

    std::cout << std::string(indent_, ' ') << text.render() << '\n';
}

void LoggerImpl::printBlank() {
    if (scopeCount_ != 0)
        lines_.push_back(packLine(Level::Blank, scopeCount_, false));

    std::cout << '\n';
}

void LoggerImpl::pushScope(LogMode mode, size_t maxDepth) {
    auto& scope = scopes_[scopeCount_];
    scope.start = static_cast<uint32_t>(lines_.size());
    scope.mode = mode;
    scope.maxDepth = maxDepth;
    scope.hadProblem = false;
    ++scopeCount_;
}

void LoggerImpl::popScope(bool success, bool hasFinal) {
    const auto& scope = scopes_[scopeCount_ - 1];
    const size_t begin = scope.start;
    const size_t end = lines_.size();
    const size_t count = end - begin;

    if (count != 0 && scopeCount_ > 1)
        std::cout << "\033[" << count << "A";

    const bool verbose = hasMode(scope.mode, LogMode::Verbose);

    auto shouldKeepLine = [&](size_t index, bool actionKept) {
        const uint8_t packed = lines_[index];
        const Level level = unpackLevel(packed);
        const bool isCurrentFinal =
            hasFinal && (index + 1 == end) && unpackFinal(packed);

        if (unpackPinned(packed) && (verbose || unpackDepth(packed) < scope.maxDepth))
            return true;

        if (level == Level::Blank)
            return LogModes::shouldKeepGap(scope.mode, actionKept);

        return LogModes::shouldKeep({
            .mode = scope.mode,
            .level = level,
            .success = success,
            .depth = unpackDepth(packed),
            .maxDepth = scope.maxDepth,
            .isCurrentFinal = isCurrentFinal,
            .isScopeFinal = unpackFinal(packed),
            .hadProblem = scope.hadProblem
        });
    };

    bool keptAction = false;
    for (size_t i = begin; i < end; ++i) {
        if (unpackLevel(lines_[i]) == Level::Action && shouldKeepLine(i, false)) {
            keptAction = true;
            break;
        }
    }

    size_t write = begin;

    for (size_t i = begin; i < end; ++i) {
        const uint8_t packed = lines_[i];
        const bool keep = shouldKeepLine(i, keptAction);

        if (keep) {
            if (scopeCount_ > 1)
                std::cout << "\r\033[1B";
            lines_[write++] = static_cast<uint8_t>(packed | kPinnedBit);
        } else {
            if (scopeCount_ > 1)
                std::cout << "\r\033[M";
        }
    }

    std::cout << std::flush;

    --scopeCount_;

    if (scopeCount_ == 0) {
        lines_.clear();
        lines_.shrink_to_fit();
    } else {
        lines_.resize(write);
    }
}

void LogSystem::setPath(const std::filesystem::path& path) {
    std::lock_guard lock(mutex_);
    path_ = path;

    if (file_.is_open())
        file_.close();

    std::filesystem::create_directories(path.parent_path());

    file_.open(path, std::ios::out | std::ios::trunc);
}
