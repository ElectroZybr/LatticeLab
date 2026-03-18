#pragma once

#include <string>
#include <string_view>
#include <cstdint>

enum class DebugDisplayType : std::uint8_t { Series, Value };

struct DebugEntry {
    DebugEntry(std::string_view label = "", DebugDisplayType type = DebugDisplayType::Series) : label(label), type(type) {}
    std::string label;
    DebugDisplayType type;
};

inline DebugEntry DebugSeries(std::string_view label) { return { label, DebugDisplayType::Series }; }
inline DebugEntry DebugValue (std::string_view label) { return { label, DebugDisplayType::Value  }; }
