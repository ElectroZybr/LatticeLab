#pragma once

#include <imgui.h>
#include "App/Signals.h"

class DebugPanel;
class FileDialogManager;
class SettingsPanel;
class IOPanel;

class ToolsPanel {
public:
    static constexpr ImGuiWindowFlags PANEL_FLAGS = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;

    void draw(float scale, DebugPanel& debug, SettingsPanel& settings, IOPanel& ioPanel);
    void setRendererType(RendererType type);

private:
    bool is3D = false;
    bool isFree = false;
};
