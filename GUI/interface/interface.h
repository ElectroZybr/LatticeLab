#pragma once
#include <memory>
#include <cstdint>
#include <string>

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>

#include "GUI/interface/file_dialog/FileDialogManager.h"
#include "GUI/interface/font_manager/FontManager.h"
#include "GUI/interface/panels/debug/DebugPanel.h"
#include "GUI/interface/panels/io/ioPanel.h"
#include "GUI/interface/panels/periodic/PeriodicPanel.h"
#include "GUI/interface/panels/settings/SettingsPanel.h"
#include "GUI/interface/panels/sim_control/SimControlPanel.h"
#include "GUI/interface/panels/stats/StatsPanel.h"
#include "GUI/interface/panels/tools/SideToolsPanel.h"
#include "GUI/interface/panels/tools/ToolsPanel.h"
#include "GUI/interface/style/StyleManager.h"
#include "GUI/interface/UiState.h"

class Interface {
public:
    Interface(sf::RenderWindow& w, Simulation& s, std::unique_ptr<IRenderer>& r, class CaptureController& c);

    int init();
    void shutdown();
    int update();
    UiState& state();
    const UiState& state() const;

    FontManager fontManager;

    DebugPanel debugPanel;
    FileDialogManager fileDialog;
    StyleManager styleManager;
    ToolsPanel toolsPanel;
    IOPanel ioPanel;
    SideToolsPanel sideToolsPanel;
    SimControlPanel simControlPanel;
    PeriodicPanel periodicPanel;
    StatsPanel statsPanel;
    SettingsPanel settingsPanel;

private:
    sf::RenderWindow* window_;
    Simulation* simulation_;
    std::unique_ptr<IRenderer>* renderer_;
    class CaptureController* captureController_;
    sf::Clock clock_;
    UiState uiState_;
};
