#include "Lattice/Log.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
std::atomic<Log::ConsoleMode> gConsoleMode = Log::ConsoleMode::Default;

struct LevelStyle {
    std::string_view label;
    std::string_view status;
    std::string_view color;
    Log::ConsoleMode consoleMode;
    bool useStdErr;
};

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

std::ofstream& logFile() {
    static std::ofstream file = [] {
        std::filesystem::create_directories("Logs");
        std::ofstream out(std::filesystem::path("Logs") / "latticelab.log", std::ios::out | std::ios::trunc);
        return out;
    }();
    return file;
}

LevelStyle levelStyle(Log::Level level) {
    using Level = Log::Level;
    switch (level) {
        case Level::Action:
            return {"ACTION", "→", LogStyle::Color::header, Log::ConsoleMode::Default, false};
        case Level::Trace:
            return {"TRACE", "·", LogStyle::Color::tree, Log::ConsoleMode::Trace, false};
        case Level::Debug:
            return {"DEBUG", "→", LogStyle::Color::header, Log::ConsoleMode::Verbose, false};
        case Level::Info:
            return {"INFO", "•", LogStyle::Color::value, Log::ConsoleMode::Verbose, false};
        case Level::Warning:
            return {"WARN", "⚠", LogStyle::Color::warning, Log::ConsoleMode::Default, true};
        case Level::Error:
            return {"ERROR", "✗", LogStyle::Color::error, Log::ConsoleMode::Default, true};
        case Level::Fatal:
            return {"FATAL", "✗", LogStyle::Color::error, Log::ConsoleMode::Default, true};
        case Level::Ok:
            return {"OK", "✓", LogStyle::Color::ok, Log::ConsoleMode::Default, false};
    }
    return {"INFO", "•", LogStyle::Color::value, Log::ConsoleMode::Verbose, false};
}

std::string makePlainLine(std::string_view label, std::string_view tag, std::string_view message) {
    return std::format("[{}] [{}] {}", label, tag, message);
}

std::string makeConsoleLine(std::string_view status, std::string_view color, std::string_view tag, std::string_view message) {
    return std::format(
        "{}{}{} [{}{}{}] {}{}{}",
        color,
        status,
        LogStyle::Color::reset,
        LogStyle::Color::bold,
        tag,
        LogStyle::Color::reset,
        LogStyle::Color::tree,
        message,
        LogStyle::Color::reset);
}
}

void Log::setConsoleMode(ConsoleMode mode) noexcept {
    gConsoleMode.store(mode, std::memory_order_relaxed);
}

Log::ConsoleMode Log::consoleMode() noexcept {
    return gConsoleMode.load(std::memory_order_relaxed);
}

std::mutex& Log::mutex() {
    static std::mutex consoleMutex;
    return consoleMutex;
}

void Log::print(Level level, std::string_view tag, const std::string& message) {
    std::lock_guard lock(mutex());
    const LevelStyle style = levelStyle(level);
    const std::string plainLine = makePlainLine(style.label, tag, message);

    std::ofstream& file = logFile();
    if (file.is_open()) {
        file << timestampForLogLine()
             << ' '
             << plainLine
             << '\n';
        file.flush();
    }

    if (static_cast<int>(consoleMode()) < static_cast<int>(style.consoleMode)) {
        return;
    }

    std::ostream& stream = style.useStdErr ? std::cerr : std::cout;
    const std::string consoleLine = makeConsoleLine(style.status, style.color, tag, message);
    stream << consoleLine << '\n';
}
