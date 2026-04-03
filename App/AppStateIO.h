#pragma once

#include <string_view>

class Simulation;
class IRenderer;

class AppStateIO {
public:
    static void save(const Simulation& simulation, const IRenderer& renderer, std::string_view path);
    static void load(Simulation& simulation, IRenderer& renderer, std::string_view path);
};
