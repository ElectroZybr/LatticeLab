#pragma once
#include <memory>
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

class Interface {
private:
    static sf::RenderWindow* window;
    static Simulation* simulation;
    static std::unique_ptr<IRenderer>* renderer;
    static sf::Clock clock;
    static int selectedAtom;
    static float simulationSpeed;
    static double averageEnergy;
    static int sim_step;

public:
    static bool pause;
    static int init(sf::RenderWindow& w, Simulation& s, std::unique_ptr<IRenderer>& r);
    static void shutdown();
    static int Update();
    static bool getPause();
    static int getSelectedAtom();
    static float getSimulationSpeed();
    static bool popStepRequested();
    static void setAverageEnergy(double energy);
    static void setSimStep(int step);
    static bool cursorHovered;
    static int countSelectedAtom;
    static bool drawToolTrip;
    static std::string toolTooltipText;

    static FontManager fontManager;

    static DebugPanel debugPanel;
    static FileDialogManager fileDialog;
    static StyleManager styleManager;
    static ToolsPanel toolsPanel;
    static IOPanel ioPanel;
    static SideToolsPanel sideToolsPanel;
    static SimControlPanel simControlPanel;
    static PeriodicPanel periodicPanel;
    static StatsPanel statsPanel;
    static SettingsPanel settingsPanel;
};
