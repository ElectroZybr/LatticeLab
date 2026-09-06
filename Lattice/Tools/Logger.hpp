#pragma once

#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <utility>
#include <vector>

#include <Lattice/Tools/LogMode.hpp>
#include <Lattice/Tools/LogStyle.hpp>
#include <Lattice/Tools/Text.hpp>


class LoggerImpl {
public:

    void print(Level level, const Text& text, bool isScopeFinal = false);
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

    void addDepth(int delta) {
        indent_ = static_cast<size_t>(static_cast<int>(indent_) + delta);
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
    static LoggerImpl& current() { return logger_; }
    static void write(Level level, const Text& text);
    static void setPath(const std::filesystem::path& path);
    static const std::filesystem::path& getPath() { return path_;}

private:
    inline static std::filesystem::path path_;
    inline static std::ofstream file_;
    inline static std::mutex mutex_;
    inline static thread_local LoggerImpl logger_;
};


namespace Logger {

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

inline void print(Level level, std::string_view tag, const Text& message, bool isScopeFinal = false) {
    LogSystem::current().print(
        level,
        Text::format(
            "{} <gr><b>[<w>{}</>]<//> {}</>",
            LogStyle::get(level).style,
            tag,
            message.markup()
        ),
        isScopeFinal
    );
}

template <typename... Args>
inline void print(Level level, std::string_view tag, std::format_string<Args...> format, Args&&... args) {
    print(level, tag, Text::format(format, std::forward<Args>(args)...));
}

template <typename... Args>
void message(std::format_string<Args...> format, Args&&... args) {
    LogSystem::current().print(Level::Message, Text::format(format, std::forward<Args>(args)...));
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
