#pragma once

#include "imgui-SFML.h"
#include <SFML/Graphics.hpp>

#include "file_dialog/FileDialogManager.h"
#include "style/StyleManager.h"
#include "panels/debug/DebugPanel.h"
#include "panels/tools/ToolsPanel.h"
#include "panels/tools/SideToolsPanel.h"
#include "panels/sim_control/SimControlPanel.h"
#include "panels/periodic/PeriodicPanel.h"
#include "panels/stats/StatsPanel.h"
#include "font_manager/FontManager.h"

class Interface {
private:
    static sf::RenderWindow* window;
    static sf::Clock clock;
    static int selectedAtom;
    static float simulationSpeed;
    static double averageEnergy;
    static int sim_step;
public:
    static bool pause;
    static int init(sf::RenderWindow& w);
    static int Update();
    static bool getPause();
    static int getSelectedAtom();
    static float getSimulationSpeed();
    static void setAverageEnergy(double energy);
    static void setSimStep(int step);
    static bool cursorHovered;
    static int countSelectedAtom;
    static bool drawToolTrip;

    static FontManager fontManager;

    static DebugPanel debugPanel;
    static FileDialogManager fileDialog;
    static StyleManager styleManager;
    static ToolsPanel toolsPanel;
    static SideToolsPanel sideToolsPanel;
    static SimControlPanel simControlPanel;
    static PeriodicPanel periodicPanel;
    static StatsPanel statsPanel;
};
