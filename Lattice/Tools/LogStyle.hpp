#pragma once

#include <string_view>

#include <Lattice/Tools/LogMode.hpp>

struct LogStyle {
    std::string_view label;
    std::string_view style;

    static const LogStyle& get(Level level);
};

inline constexpr LogStyle logStyles[] = {
    {"",          "<mut>"},
    {"OK",        "<ok>✓"},
    {"ACTION",    "<a2>➜"},
    {"INFO",      "<mut>•"},
    {"WARN",      "<wrn>⚠"},
    {"ERROR",     "<err>⚠"},
    {"EXCEPTION", "<err>✗"},
};

inline const LogStyle& LogStyle::get(Level level) {
    return logStyles[static_cast<size_t>(level)];
}
