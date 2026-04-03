#include "AppStateIO.h"

#include <fstream>
#include <string>

#include "Engine/Simulation.h"
#include "Engine/io/SimulationStateIO.h"
#include "Rendering/BaseRenderer.h"

void AppStateIO::save(const Simulation& simulation, const IRenderer& renderer, std::string_view path) {
    SimulationStateIO::save(simulation, path);

    std::ofstream file(path.data(), std::ios::app);
    if (!file.is_open()) {
        return;
    }

    file << "draw_grid " << static_cast<int>(renderer.drawGrid) << "\n";
    file << "draw_bonds " << static_cast<int>(renderer.drawBonds) << "\n";
    file << "speed_color_mode " << static_cast<int>(renderer.speedColorMode) << "\n";
    file << "speed_gradient_max " << renderer.speedGradientMax << "\n";
    file << "renderer_alpha " << renderer.alpha << "\n";
}

void AppStateIO::load(Simulation& simulation, IRenderer& renderer, std::string_view path) {
    SimulationStateIO::load(simulation, path);

    std::ifstream file(path.data());
    if (!file.is_open()) {
        return;
    }

    bool loadedDrawGrid = renderer.drawGrid;
    bool loadedDrawBonds = renderer.drawBonds;
    int loadedSpeedColorMode = static_cast<int>(renderer.speedColorMode);
    float loadedSpeedGradientMax = renderer.speedGradientMax;
    float loadedRendererAlpha = renderer.alpha;

    std::string tag;
    while (file >> tag) {
        if (tag == "draw_grid") {
            int value = 0;
            file >> value;
            loadedDrawGrid = (value != 0);
        } else if (tag == "draw_bonds") {
            int value = 0;
            file >> value;
            loadedDrawBonds = (value != 0);
        } else if (tag == "speed_color_mode") {
            file >> loadedSpeedColorMode;
        } else if (tag == "speed_gradient_max") {
            file >> loadedSpeedGradientMax;
        } else if (tag == "renderer_alpha") {
            file >> loadedRendererAlpha;
        } else if (tag == "atom") {
            std::string atomLine;
            std::getline(file, atomLine);
        } else {
            std::string ignoredLine;
            std::getline(file, ignoredLine);
        }
    }

    renderer.drawGrid = loadedDrawGrid;
    renderer.drawBonds = loadedDrawBonds;
    renderer.speedColorMode = static_cast<IRenderer::SpeedColorMode>(loadedSpeedColorMode);
    renderer.speedGradientMax = loadedSpeedGradientMax;
    renderer.alpha = loadedRendererAlpha;
}
