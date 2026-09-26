#pragma once

#include <cstdint>
#include <string>

namespace CLIPlugin {

enum class CursorStyle : uint8_t {
    Default = 0,
    BlinkingBlock = 1,
    Block = 2,
    BlinkingUnderline = 3,
    Underline = 4,
    BlinkingBar = 5,
    Bar = 6
};

struct TerminalStyle {
    std::string prompt = "❯";
    std::string promptColor = "\033[95m";
    std::string pathColor = "\033[90m";
    CursorStyle cursor = CursorStyle::BlinkingBar;
    bool showPath = true;
};

}