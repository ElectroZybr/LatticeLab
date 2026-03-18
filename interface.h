#pragma once

#include "imgui.h"
#include "imgui-SFML.h"
#include <SFML/Graphics.hpp>

#include "Engine/gui/interface/debug_panel/DebugPanel.h"
#include "Engine/gui/interface/file_dialog/FileDialogManager.h"
#include "Engine/gui/interface/style/StyleManager.h"

class Interface {
private:
    static sf::RenderWindow* window;
    static sf::Clock clock;
    static ImGuiStyle* style;
    static ImGuiStyle baseStyle;
    static ImFont* Rubik_VariableFont_wght;
    static ImFont* Font_Awesome;
    static ImFont* DialogFont;
    static int selectedAtom;
    static float simulationSpeed;
    static double averageEnergy;
    static int sim_step;
public:
    static bool pause;
    static int init(sf::RenderWindow& w);
    static int Update();
    static void CheckEvent(const sf::Event& event);
    static bool getPause();
    static int getSelectedAtom();
    static float getSimulationSpeed();
    static void setAverageEnergy(double energy);
    static void setSimStep(int step);
    static bool cursorHovered;
    static int countSelectedAtom;
    static bool drawToolTrip;
    static DebugPanel debugPanel;
    static FileDialogManager fileDialog;
    static StyleManager styleManager;
};
