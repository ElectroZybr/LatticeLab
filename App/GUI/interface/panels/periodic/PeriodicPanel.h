#pragma once
#include <imgui.h>
#include <glm/vec2.hpp>
#include <string>

namespace Lattice {
    class Simulation;
}

class PeriodicPanel {
public:
    static constexpr ImGuiWindowFlags PANEL_FLAGS = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;

    void draw(float scale, glm::ivec2 windowSize, const Lattice::Simulation& simulation, int& selectedAtom, std::string& spawnSpecies);

    static int decodeAtom(int index);

private:
    float animProgress = 0.0f;
};
