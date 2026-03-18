#pragma once
#include <vector>
#include <string_view>

#include <SFML/Graphics.hpp>
#include "imgui.h"

#include "view/DebugView.h"

class DebugPanel {
    std::vector<DebugView> views;
    bool visible = false;
    float animProgress = 0.f;
    ImFont* font = nullptr;

public:
    DebugView* addView(DebugView view);

    void draw(float uiScale, sf::Vector2u windowSize);
    void loadFont(std::string_view path, float size);

    void toggle() { visible = !visible; }
    bool isVisible() const { return visible; }
};