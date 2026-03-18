#pragma once
#include "imgui.h"

class FontManager {
public:
    bool load();

    ImFont* main   = nullptr;
    ImFont* dialog = nullptr;
};