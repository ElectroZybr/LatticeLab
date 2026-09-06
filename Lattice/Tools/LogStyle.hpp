#pragma once

#include <string_view>

#include <Lattice/Tools/Text.hpp>
#include <Lattice/Tools/LogMode.hpp>

struct LogStyle {
    std::string_view label;
    std::string_view style;

    static const LogStyle& get(Level level);
};

inline constexpr LogStyle logStyles[] = {
    {"",          "<gr>"},
    {"OK",        "<g>✓"},
    {"ACTION",    "<c>➜"},
    {"INFO",      "<gr>•"},
    {"WARN",      "<y>⚠"},
    {"ERROR",     "<r>⚠"},
    {"EXCEPTION", "<r>✗"},
    {"",          ""},
};

inline const LogStyle& LogStyle::get(Level level) {
    return logStyles[static_cast<size_t>(level)];
}