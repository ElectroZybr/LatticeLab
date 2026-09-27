#pragma once

#include <cstdint>
#include <string>

#include <Lattice/Tools/TextStyle.hpp>

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
    Lattice::TextStyle promptColor = Lattice::TextStyle::rgb(0xff55ff);
    Lattice::TextStyle pathColor = Lattice::TextStyle::rgb(0xaaaaaa);
    CursorStyle cursor = CursorStyle::BlinkingBar;
    bool showPath = true;
};

}
