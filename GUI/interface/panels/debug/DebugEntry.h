#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <any>

#include "DebugDrawers.h"

enum class DebugDisplayType : uint8_t { Series, Value };

struct DebugEntry {
    using DrawFn = void(*)(std::string_view label, const std::any& value, int precision);

    DebugEntry(std::string_view label = "", DebugDisplayType type = DebugDisplayType::Series,
               DrawFn draw = DebugDrawers::Float, int precision = 2)
        : label(label), type(type), draw(draw), precision(precision) {}
    std::string label;
    DebugDisplayType type;
    DrawFn draw;
    int precision;
};

inline DebugEntry DebugSeries(std::string_view label, int precision = 2) {
    return { label, DebugDisplayType::Series, DebugDrawers::Float, precision };
}
inline DebugEntry DebugValue(std::string_view label, DebugEntry::DrawFn draw = DebugDrawers::Float, int precision = 2) {
    return { label, DebugDisplayType::Value, draw, precision };
}
