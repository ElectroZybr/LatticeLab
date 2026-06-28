#pragma once

#include <chrono>
#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

#include "Lattice/LogStyle.h"

class Log {
public:
    enum class ConsoleMode {
        Quiet,
        Default,
        Verbose,
        Trace,
    };

    enum class Level {
        Action,
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal,
        Ok,
    };

    static void setConsoleMode(ConsoleMode mode) noexcept;
    static ConsoleMode consoleMode() noexcept;

    template <typename... TArgs>
    static void action(std::string_view tag, std::format_string<TArgs...> format, TArgs&&... args) {
        print(Level::Action, tag, std::format(format, std::forward<TArgs>(args)...));
    }

    template <typename... TArgs>
    static void trace(std::string_view tag, std::format_string<TArgs...> format, TArgs&&... args) {
        print(Level::Trace, tag, std::format(format, std::forward<TArgs>(args)...));
    }

    template <typename... TArgs>
    static void debug(std::string_view tag, std::format_string<TArgs...> format, TArgs&&... args) {
        print(Level::Debug, tag, std::format(format, std::forward<TArgs>(args)...));
    }

    template <typename... TArgs>
    static void info(std::string_view tag, std::format_string<TArgs...> format, TArgs&&... args) {
        print(Level::Info, tag, std::format(format, std::forward<TArgs>(args)...));
    }

    template <typename... TArgs>
    static void warning(std::string_view tag, std::format_string<TArgs...> format, TArgs&&... args) {
        print(Level::Warning, tag, std::format(format, std::forward<TArgs>(args)...));
    }

    template <typename... TArgs>
    static void error(std::string_view tag, std::format_string<TArgs...> format, TArgs&&... args) {
        print(Level::Error, tag, std::format(format, std::forward<TArgs>(args)...));
    }

    template <typename... TArgs>
    static void fatal(std::string_view tag, std::format_string<TArgs...> format, TArgs&&... args) {
        print(Level::Fatal, tag, std::format(format, std::forward<TArgs>(args)...));
    }

    template <typename... TArgs>
    static void ok(std::string_view tag, std::format_string<TArgs...> format, TArgs&&... args) {
        print(Level::Ok, tag, std::format(format, std::forward<TArgs>(args)...));
    }

private:
    static void print(Level level, std::string_view tag, const std::string& message);
    static std::mutex& mutex();
};

class LogScope {
public:
    LogScope(std::string_view tag, std::string_view startMessage, std::string_view finishMessage = "Initialized")
        : tag_(tag),
          finishMessage_(finishMessage),
          startTime_(Clock::now()),
          active_(true) {
        Log::action(tag_, "{}", startMessage);
    }

    LogScope(const LogScope&) = delete;
    LogScope& operator=(const LogScope&) = delete;

    LogScope(LogScope&& other) noexcept
        : tag_(other.tag_),
          finishMessage_(other.finishMessage_),
          startTime_(other.startTime_),
          active_(other.active_) {
        other.active_ = false;
    }

    LogScope& operator=(LogScope&&) = delete;

    ~LogScope() {
        if (active_) {
            finish();
        }
    }

    template <typename... TArgs>
    void step(std::format_string<TArgs...> format, TArgs&&... args) const {
        Log::info(tag_, format, std::forward<TArgs>(args)...);
    }

    void finish() noexcept {
        if (!active_) {
            return;
        }

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - startTime_).count();
        Log::ok(tag_, "{} ({} ms)", finishMessage_, elapsed);
        active_ = false;
    }

    void cancel() noexcept { active_ = false; }

private:
    using Clock = std::chrono::steady_clock;

    std::string tag_;
    std::string finishMessage_;
    Clock::time_point startTime_;
    bool active_;
};
