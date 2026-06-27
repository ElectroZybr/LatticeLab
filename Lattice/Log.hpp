#pragma once

#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

#include "Lattice/LogStyle.h"

class Log {
public:
    static void setVerbose(bool enabled) noexcept;
    static bool isVerbose() noexcept;

    static std::string highlight(std::string_view text) {
        std::string out;
        out.reserve(LogStyle::Color::value.size() + text.size() + LogStyle::Color::tree.size());
        out += LogStyle::Color::value;
        out += text;
        out += LogStyle::Color::tree;
        return out;
    }

    template <typename... TArgs>
    static std::string highlight(std::format_string<TArgs...> format, TArgs&&... args) {
        return highlight(std::format(format, std::forward<TArgs>(args)...));
    }

    template <typename... TArgs>
    static void write(std::string_view tag, std::format_string<TArgs...> format, TArgs&&... args) {
        print(std::cout, "→", LogStyle::Color::header, tag, std::format(format, std::forward<TArgs>(args)...));
    }

    template <typename... TArgs>
    static void info(std::string_view tag, std::format_string<TArgs...> format, TArgs&&... args) {
        if (!isVerbose()) {
            return;
        }
        print(std::cout, "•", LogStyle::Color::value, tag, std::format(format, std::forward<TArgs>(args)...));
    }

    template <typename... TArgs>
    static void warning(std::string_view tag, std::format_string<TArgs...> format, TArgs&&... args) {
        print(std::cerr, "⚠", LogStyle::Color::warning, tag, std::format(format, std::forward<TArgs>(args)...));
    }

    template <typename... TArgs>
    static void warn(std::string_view tag, std::format_string<TArgs...> format, TArgs&&... args) {
        warning(tag, format, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    static void error(std::string_view tag, std::format_string<TArgs...> format, TArgs&&... args) {
        print(std::cerr, "✗", LogStyle::Color::error, tag, std::format(format, std::forward<TArgs>(args)...));
    }

    template <typename... TArgs>
    static void ok(std::string_view tag, std::format_string<TArgs...> format, TArgs&&... args) {
        print(std::cout, "✓", LogStyle::Color::ok, tag, std::format(format, std::forward<TArgs>(args)...));
    }

private:
    static void print(std::ostream& stream, std::string_view status, std::string_view color, std::string_view tag, const std::string& message);
    static std::mutex& mutex();
};
