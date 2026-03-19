#pragma once

#include <SFML/Graphics.hpp>

#include "Engine/physics/Atom.h"
#include "Engine/SimBox.h"
#include "Rendering/BaseRenderer.h"

class Mouse {
    friend class EventManager;
public:
    static void init(sf::RenderWindow* w, IRenderer* r, SimBox* b, std::vector<Atom>* a);

    static void onEvent(const sf::Event& event);
    static void onFrame();

private:
    static void onLeftPressed(sf::Vector2i mouse_pos);
    static void onLeftReleased();

    static Atom* findAtomAt(Vec3D local);

    static sf::RenderWindow* window;
    static IRenderer*        render;
    static SimBox*           box;
    static std::vector<Atom>* atoms;

    static bool atomMoveFlag;
    static bool selectionFrameMoveFlag;
    static Atom* selectedMoveAtom;
    static sf::Vector2i start_mouse_pos;
};
