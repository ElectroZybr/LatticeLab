#pragma once
#include <deque>

#include <SFML/Graphics.hpp>

#include "GUI/interface/panels/debug/view/DebugView.h"

class DebugPanel {
    std::deque<DebugView> views;
    bool visible = false;
    float animProgress = 0.f;

public:
    DebugView* addView(DebugView view);

    void draw(float uiScale, Vec2u windowSize);

    void toggle() { visible = !visible; }
    void close() { visible = false; }
    bool isVisible() const { return visible; }
};
