// #ifndef interface
// #define interface
#pragma once

#include "imgui.h"
#include "imgui-SFML.h"
#include <SFML/Graphics.hpp>

class Interface {
private:
    static sf::RenderWindow* window;
    static sf::Clock clock;
    static ImGuiStyle* style;
    static ImGuiStyle baseStyle;
    static ImFont* Rubik_VariableFont_wght;
    static ImFont* Font_Awesome;
    static float current_ui_scale;
    static int selectedAtom;
    static float simulationSpeed;
    static double averageEnergy;
    static int sim_step;
public:
    static bool pause;
    static int init(sf::RenderWindow& w);
    static void custom_style();
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
};

// #endif
