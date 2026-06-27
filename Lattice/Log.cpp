#include "Lattice/Log.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
struct LevelStyle {
    std::string_view label;
    std::string_view status;
    std::string_view color;
    bool consoleVisible;
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
        case Level::Trace:
            return {"TRACE", "·", LogStyle::Color::tree, false, false};
        case Level::Debug:
            return {"DEBUG", "→", LogStyle::Color::header, false, false};
        case Level::Info:
            return {"INFO", "•", LogStyle::Color::value, true, false};
        case Level::Warning:
            return {"WARN", "⚠", LogStyle::Color::warning, true, true};
        case Level::Error:
            return {"ERROR", "✗", LogStyle::Color::error, true, true};
        case Level::Fatal:
            return {"FATAL", "✗", LogStyle::Color::error, true, true};
        case Level::Ok:
            return {"OK", "✓", LogStyle::Color::ok, true, false};
    }
    return {"INFO", "•", LogStyle::Color::value, true, false};
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

    if (!style.consoleVisible) {
        return;
    }

    std::ostream& stream = style.useStdErr ? std::cerr : std::cout;
    const std::string consoleLine = makeConsoleLine(style.status, style.color, tag, message);
    stream << consoleLine << '\n';
}
