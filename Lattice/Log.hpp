#pragma once

#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

#include "Lattice/LogStyle.h"

class Log {
public:
    enum class Level {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal,
        Ok,
    };

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
