#pragma once
#include <cstdint>
#include <memory>
#include <optional>

#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics.hpp>

class Simulation;
class IRenderer;

enum class SettingsCommand : uint8_t {
    ExitApplication,
};

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
    std::optional<SettingsCommand> popResult();

private:
    bool  visible      = false;
    float animProgress = 0.f;
    std::optional<SettingsCommand> pendingResult_;
};
