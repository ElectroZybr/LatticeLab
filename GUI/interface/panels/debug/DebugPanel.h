#pragma once
#include <deque>

#include <SFML/Graphics.hpp>

#include "view/DebugView.h"

class DebugPanel {
    std::deque<DebugView> views;
    bool visible = false;
    float animProgress = 0.f;

public:
    DebugView* addView(DebugView view);

    void draw(float uiScale, sf::Vector2u windowSize);

    void toggle() { visible = !visible; }
    void close() { visible = false; }
    bool isVisible() const { return visible; }
};
