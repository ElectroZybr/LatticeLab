#pragma once
#include <memory>

#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics.hpp>

class Simulation;
class IRenderer;

class SettingsPanel {
public:
    static constexpr ImGuiWindowFlags PANEL_FLAGS =
        ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar;

    void draw(float uiScale, sf::Vector2u windowSize, Simulation& simulation, std::unique_ptr<IRenderer>& renderer);
    void toggle() { visible = !visible; }
    void close() { visible = false; }
    bool isVisible() const { return visible; }
private:
    bool  visible      = false;
    float animProgress = 0.f;
};
