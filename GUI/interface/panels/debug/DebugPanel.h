#pragma once
#include <vector>

#include <SFML/Graphics.hpp>

#include "view/DebugView.h"

class DebugPanel {
    std::vector<DebugView> views;
    bool visible = false;
    float animProgress = 0.f;
public:
    DebugView* addView(DebugView view);

    void draw(float uiScale, sf::Vector2u windowSize);

    void toggle() { visible = !visible; }
    bool isVisible() const { return visible; }
};