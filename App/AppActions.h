#pragma once

#include <memory>

#include "Scenes.h"
#include "Engine/Simulation.h"
#include "Engine/tools/Tools.h"
#include "GUI/interface/interface.h"
#include "Rendering/2d/Renderer2D.h"
#include "Rendering/3d/Renderer3D.h"

namespace sf {
class RenderWindow;
class View;
}

inline void processFileDialog(Simulation& simulation) {
    if (auto result = Interface::fileDialog.popResult()) {
        switch (result->command) {
            case FileDialogCommand::Save:
                simulation.save(result->path);
                break;
            case FileDialogCommand::Load:
                simulation.load(result->path);
                Tools::resetInteractionState();
                break;
        }
    }
}

inline void processToolsPanel(std::unique_ptr<IRenderer>& renderer, sf::RenderWindow& window,
                              sf::View& gameView, Simulation& simulation) {
    if (auto result = Interface::toolsPanel.popResult()) {
        std::unique_ptr<IRenderer> newRenderer;
        switch (result.value()) {
            case ToolsCommand::ToggleRenderer2D:
                newRenderer = std::make_unique<Renderer2D>(window, gameView, simulation.sim_box);
                break;
            case ToolsCommand::ToggleRenderer3D:
                newRenderer = std::make_unique<Renderer3D>(window, gameView, simulation.sim_box);
                break;
            case ToolsCommand::ClearSimulation:
                simulation.clear();
                Tools::resetInteractionState();
                break;
            case ToolsCommand::SetCameraOrbit:
                renderer->camera.setMode(Camera::Mode::Orbit);
                break;
            case ToolsCommand::SetCameraFree:
                renderer->camera.setMode(Camera::Mode::Free);
                break;
        }

        if (newRenderer) {
            newRenderer->drawGrid = renderer->drawGrid;
            newRenderer->drawBonds = renderer->drawBonds;
            newRenderer->speedColorMode = renderer->speedColorMode;
            newRenderer->speedGradientMax = renderer->speedGradientMax;
            newRenderer->setAtomStorage(&simulation.atomStorage);
            renderer = std::move(newRenderer);
        }
    }
}

inline void applyResizeBox(Simulation& simulation,
                           std::unique_ptr<IRenderer>& renderer,
                           const Vec3f& newSize,
                           const Vec3f& oldSize,
                           bool resizeGrid = true,
                           bool moveCamera = true,
                           bool moveAtoms = true) {
    if (resizeGrid) {
        simulation.setSizeBox(newSize);
    }

    const Vec3f delta = (newSize - oldSize) * 0.5f;

    if (moveCamera) {
        renderer->camera.move3D(delta);
        renderer->camera.move({delta.x, delta.y});
    }

    if (moveAtoms) {
        float* x = simulation.atomStorage.xData();
        float* y = simulation.atomStorage.yData();
        float* z = simulation.atomStorage.zData();
        for (size_t i = 0; i < simulation.atomStorage.size(); ++i) {
            x[i] += delta.x;
            y[i] += delta.y;
            z[i] += delta.z;
        }
    }
}

inline void processIOPanel(Simulation& simulation, std::unique_ptr<IRenderer>& renderer) {
    if (auto result = Interface::ioPanel.popResult()) {
        switch (result.value()) {
            case IOCommand::ApplyBoxSize: {
                applyResizeBox(
                    simulation,
                    renderer,
                    Interface::ioPanel.boxSize(),
                    Vec3f(simulation.sim_box.size)
                );
                break;
            }
            case IOCommand::CreateCrystal: {
                simulation.clear();
                const Vec3f oldSize = simulation.sim_box.size;
                Scenes::crystal(simulation, Interface::ioPanel.sceneAxisCount(), Interface::ioPanel.atomType(), Interface::ioPanel.sceneIs3D());
                Tools::resetInteractionState();
                applyResizeBox(simulation, renderer, simulation.sim_box.size, oldSize, false, true, false);
                break;
            }
            case IOCommand::CreateGas: {
                simulation.clear();
                const Vec3f oldSize = simulation.sim_box.size;
                Scenes::randomGas(simulation,
                                  Interface::ioPanel.gasAtomCount(),
                                  Interface::ioPanel.gasAtomType(),
                                  Interface::ioPanel.gasIs3D(),
                                  6.0,
                                  6.0,
                                  Interface::ioPanel.gasDensity());
                Tools::resetInteractionState();
                applyResizeBox(simulation, renderer, simulation.sim_box.size, oldSize, false, true, false);
                break;
            }
            case IOCommand::ClearSimulation:
                simulation.clear();
                Tools::resetInteractionState();
                break;
        }
    }
}

inline void processSettingsPanel(sf::RenderWindow& window) {
    if (auto result = Interface::settingsPanel.popResult()) {
        switch (result.value()) {
            case SettingsCommand::ExitApplication:
                window.close();
                break;
        }
    }
}
