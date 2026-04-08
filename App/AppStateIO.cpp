#include "AppStateIO.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include "Engine/io/SimulationStateIO.h"
#include "Rendering/BaseRenderer.h"

namespace {
    constexpr const char* kBlockIndent = "  ";

    struct LoadedRendererData {
        bool drawGrid = false;
        bool drawBonds = false;
        int speedColorMode = static_cast<int>(IRenderer::SpeedColorMode::AtomColor);
        float speedGradientMax = 5.0f;
        float alpha = 0.05f;
    };

    std::string trim(std::string_view value) {
        size_t begin = 0;
        while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
            ++begin;
        }

        size_t end = value.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
            --end;
        }

        return std::string(value.substr(begin, end - begin));
    }

    void saveRendererState(const IRenderer& renderer, std::string_view path) {
        std::ofstream file(path.data(), std::ios::app);
        if (!file.is_open()) {
            return;
        }

        file << "\n[renderer]\n";
        file << kBlockIndent << "draw_grid " << static_cast<int>(renderer.drawGrid) << "\n";
        file << kBlockIndent << "draw_bonds " << static_cast<int>(renderer.drawBonds) << "\n";
        file << kBlockIndent << "speed_color_mode " << static_cast<int>(renderer.speedColorMode) << "\n";
        file << kBlockIndent << "speed_gradient_max " << renderer.speedGradientMax << "\n";
        file << kBlockIndent << "renderer_alpha " << renderer.alpha << "\n";
    }

    void loadRendererState(IRenderer& renderer, std::string_view path) {
        std::ifstream file(path.data());
        if (!file.is_open()) {
            return;
        }

        LoadedRendererData loadedRenderer{
            .drawGrid = renderer.drawGrid,
            .drawBonds = renderer.drawBonds,
            .speedColorMode = static_cast<int>(renderer.speedColorMode),
            .speedGradientMax = renderer.speedGradientMax,
            .alpha = renderer.alpha,
        };

        std::string line;
        while (std::getline(file, line)) {
            const std::string trimmed = trim(line);
            if (trimmed.empty() || trimmed.front() == '#' || trimmed.front() == '[') {
                continue;
            }

            std::istringstream stream(trimmed);
            std::string tag;
            stream >> tag;

            if (tag == "draw_grid") {
                int value = 0;
                stream >> value;
                loadedRenderer.drawGrid = (value != 0);
            }
            else if (tag == "draw_bonds") {
                int value = 0;
                stream >> value;
                loadedRenderer.drawBonds = (value != 0);
            }
            else if (tag == "speed_color_mode") {
                stream >> loadedRenderer.speedColorMode;
            }
            else if (tag == "speed_gradient_max") {
                stream >> loadedRenderer.speedGradientMax;
            }
            else if (tag == "renderer_alpha") {
                stream >> loadedRenderer.alpha;
            }
        }

        renderer.drawGrid = loadedRenderer.drawGrid;
        renderer.drawBonds = loadedRenderer.drawBonds;
        renderer.speedColorMode = static_cast<IRenderer::SpeedColorMode>(loadedRenderer.speedColorMode);
        renderer.speedGradientMax = loadedRenderer.speedGradientMax;
        renderer.alpha = loadedRenderer.alpha;
    }
}

void AppStateIO::save(const Simulation& simulation, const IRenderer& renderer, std::string_view path) {
    SimulationStateIO::save(simulation, path);
    saveRendererState(renderer, path);
}

void AppStateIO::load(Simulation& simulation, IRenderer& renderer, std::string_view path) {
    SimulationStateIO::load(simulation, path);
    loadRendererState(renderer, path);
}
