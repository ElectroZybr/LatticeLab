#pragma once

#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <Lattice/Tools/LogMode.hpp>
#include <Lattice/Tools/LogStyle.hpp>
#include <Lattice/Tools/TextFormatter.hpp>


struct LogEvent {
    Level level;
    TextFormatter text;
};

class LoggerImpl {
public:
    void print(Level level, const TextFormatter& text, bool isScopeFinal = false);
    void printBlank();
    void pushScope(LogMode mode, size_t maxDepth);
    void popScope(bool success, bool hasFinal = true);

    void setDefaultMode(LogMode mode) { defaultMode_ = mode; }
    void addDefaultMode(LogMode mode) { defaultMode_ |= mode; }
    void setMaxDepth(size_t depth) { defaultMaxDepth_ = depth; }

    LogMode defaultMode() const { return defaultMode_; }
    size_t defaultMaxDepth() const { return defaultMaxDepth_; }

    LogMode currentScopeMode() const {
        return scopes_[scopeCount_ - 1].mode;
    }

    LogMode currentOrDefault() const {
        return scopeCount_ != 0 ? scopes_[scopeCount_ - 1].mode : defaultMode_;
    }

    size_t scopeDepth() const { return scopeCount_; }

    bool currentHadProblem() const {
        return scopeCount_ != 0 && scopes_[scopeCount_ - 1].hadProblem;
    }

    void addDepth(size_t delta) {
        indent_ += delta;
    }

private:

    static constexpr uint8_t MaxScopes = 32;

    struct Scope {
        uint32_t start = 0;
        LogMode mode = LogModes::Default;
        size_t maxDepth = LogModes::UnlimitedDepth;
        bool hadProblem = false;
    };

    size_t indent_ = 0;
    uint8_t scopeCount_ = 0;
    Scope scopes_[MaxScopes];
    std::vector<uint8_t> lines_;
    LogMode defaultMode_ = LogModes::Default;
    size_t defaultMaxDepth_ = LogModes::UnlimitedDepth;
};


class LogSystem {
public:
    using SinkId = uint64_t;
    using Sink = std::function<void(const LogEvent&)>;
    using ConsoleWriterId = uint64_t;
    using ConsoleWriter = std::function<void(std::string_view)>;

    static LoggerImpl& current() { return logger_; }

    static void write(Level level, const TextFormatter& text);

    static SinkId addSink(Sink sink);
    static void removeSink(SinkId id);

    // A console writer changes only terminal presentation. File output and log
    // sinks continue to receive the original events.
    static ConsoleWriterId setConsoleWriter(ConsoleWriter writer);
    static void resetConsoleWriter(ConsoleWriterId id);
    static void writeConsole(std::string_view text);

    static void setPath(const std::filesystem::path& path);
    static const std::filesystem::path& getPath() { return path_; }

private:
    inline static std::filesystem::path path_;
    inline static std::ofstream file_;
    inline static std::mutex mutex_;
    inline static std::mutex consoleMutex_;
    inline static thread_local LoggerImpl logger_;

    inline static std::unordered_map<SinkId, Sink> sinks_;
    inline static SinkId nextSinkId_ = 1;
    inline static ConsoleWriter consoleWriter_;
    inline static ConsoleWriterId consoleWriterId_ = 0;
    inline static ConsoleWriterId nextConsoleWriterId_ = 1;
};


namespace Logger {

inline TextFormatter line(Level level, std::string_view tag, const TextFormatter& message) {
    return TextFormatter::format(
        "{} <mut><b>[<light>{}</>]<//> {}</>",
        LogStyle::get(level).style,
        tag,
        message.markup()
    );
}

inline void reportDiagnostics(const TextFormatter& text) {
    for (const Lattice::TextDiagnostic& diagnostic : text.diagnostics()) {
        TextFormatter details;
        details.append(std::format(
            "{} at {}:{}: {}\n  {}\n  {}^",
            diagnostic.owner,
            diagnostic.lineNumber,
            diagnostic.column,
            diagnostic.message,
            diagnostic.line,
            std::string(diagnostic.column - 1, ' ')
        ));
        LogSystem::current().print(
            Level::Warning,
            line(Level::Warning, "Text", details)
        );
    }
}

inline void setDefaultMode(LogMode mode) {
    LogSystem::current().setDefaultMode(mode);
}

inline void addDefaultMode(LogMode mode) {
    LogSystem::current().addDefaultMode(mode);
}

inline void setMaxDepth(size_t depth) {
    LogSystem::current().setMaxDepth(depth);
}

inline void blank() {
    LogSystem::current().printBlank();
}

inline void message(const TextFormatter& text) {
    reportDiagnostics(text);
    LogSystem::current().print(Level::Message, text);
}

inline void print(Level level, std::string_view tag, const TextFormatter& message, bool isScopeFinal = false) {
    TextFormatter output = line(level, tag, message);
    reportDiagnostics(output);
    LogSystem::current().print(level, output, isScopeFinal);
}

template <typename... Args>
inline void print(Level level, std::string_view tag, std::format_string<Args...> format, Args&&... args) {
    print(level, tag, TextFormatter::format(format, std::forward<Args>(args)...));
}

template <typename... Args>
void message(std::format_string<Args...> format, Args&&... args) {
    message(TextFormatter::format(format, std::forward<Args>(args)...));
}

template <typename... Args>
void ok(std::string_view tag, std::format_string<Args...> format, Args&&... args) {
    print(Level::Ok, tag, format, std::forward<Args>(args)...);
}

template <typename... Args>
void action(std::string_view tag, std::format_string<Args...> format, Args&&... args) {
    print(Level::Action, tag, format, std::forward<Args>(args)...);
}

template <typename... Args>
void info(std::string_view tag, std::format_string<Args...> format, Args&&... args) {
    print(Level::Info, tag, format, std::forward<Args>(args)...);
}

template <typename... Args>
void warning(std::string_view tag, std::format_string<Args...> format, Args&&... args) {
    print(Level::Warning, tag, format, std::forward<Args>(args)...);
}

template <typename... Args>
void error(std::string_view tag, std::format_string<Args...> format, Args&&... args) {
    print(Level::Error, tag, format, std::forward<Args>(args)...);
}

template <typename... Args>
void exception(std::string_view tag, std::format_string<Args...> format, Args&&... args) {
    print(Level::Exception, tag, format, std::forward<Args>(args)...);
}

}
