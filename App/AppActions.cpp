#include "AppActions.h"

#include "App/AppSignals.h"
#include "App/AppStateIO.h"
#include "App/Scenes.h"
#include "App/interaction/ToolsManager.h"
#include "Engine/Simulation.h"
#include "GUI/interface/UiState.h"
#include "Rendering/2d/Renderer2D.h"
#include "Rendering/3d/Renderer3D.h"

namespace {
    void shiftAtoms(AtomStorage& atomStorage, Vec3f delta) {
        float* x = atomStorage.xData();
        float* y = atomStorage.yData();
        float* z = atomStorage.zData();
        const size_t atomCount = atomStorage.size();

        for (size_t i = 0; i < atomCount; ++i) {
            x[i] += delta.x;
            y[i] += delta.y;
            z[i] += delta.z;
        }
    }

    void applyResizeBox(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, const Vec3f& newSize) {
        const Vec3f oldSize = simulation.box().size;
        const Vec3f delta = (newSize - oldSize) * 0.5f;

        shiftAtoms(simulation.atoms(), delta);
        renderer->camera.move({delta.x, delta.y});
        renderer->camera.move3D(delta);
        simulation.setSizeBox(newSize);
    }
}

namespace AppActions {
    void Handler::trackIOPanel(sf::RenderWindow& window, UiState& uiState, Simulation& simulation, std::unique_ptr<IRenderer>& renderer) {
        track(AppSignals::UI::SaveSimulation.connect(
            [&](std::string_view path) { AppStateIO::save(window, uiState.scenePreviewRect, simulation, *renderer, path); }));
        track(AppSignals::UI::LoadSimulation.connect([&](std::string_view path) {
            AppStateIO::load(simulation, *renderer, path);
            ToolsManager::resetInteractionState();
        }));
        track(AppSignals::UI::ResizeBox.connect([&](const Vec3f& newSize) { applyResizeBox(simulation, renderer, newSize); }));
        track(AppSignals::UI::ClearSimulation.connect([&]() {
            simulation.clear();
            ToolsManager::resetInteractionState();
        }));
        track(AppSignals::UI::CreateGas.connect([&](int atomCount, AtomData::Type atomType, bool is3D, float density) {
            simulation.clear();
            ToolsManager::resetInteractionState();
            Scenes::randomGas(simulation, atomCount, atomType, is3D, 6.0f, 6.0f, density);
        }));
        track(AppSignals::UI::CreateCrystal.connect([&](int axisCount, AtomData::Type atomType, bool is3D) {
            simulation.clear();
            ToolsManager::resetInteractionState();
            Scenes::crystal(simulation, axisCount, atomType, is3D);
        }));
    }

    void Handler::trackToolsPanel(Simulation& simulation, std::unique_ptr<IRenderer>& renderer, sf::RenderWindow& window,
                                  sf::View& sceneView) {
        track(AppSignals::UI::SetRender.connect([&](RendererType type) {
            std::unique_ptr<IRenderer> newRenderer;
            switch (type) {
            case RendererType::Renderer2D:
                newRenderer = std::make_unique<Renderer2D>(window, sceneView, simulation.box());
                break;
            case RendererType::Renderer3D:
                newRenderer = std::make_unique<Renderer3D>(window, sceneView, simulation.box());
                break;
            }

            if (newRenderer) {
                ToolsManager::resetInteractionState();
                newRenderer->drawGrid = renderer->drawGrid;
                newRenderer->drawBonds = renderer->drawBonds;
                newRenderer->speedColorMode = renderer->speedColorMode;
                newRenderer->speedGradientMax = renderer->speedGradientMax;
                renderer = std::move(newRenderer);
            }
        }));

        track(AppSignals::UI::SetCameraMode.connect([&](Camera::Mode mode) { renderer->camera.setMode(mode); }));
    }

    void Handler::trackSettingsPanel(sf::Window& window) {
        track(AppSignals::UI::ExitApplication.connect(&sf::Window::close, &window));
    }

    void Handler::trackKeyboard(Simulation& simulation) {
        track(AppSignals::Keyboard::StepPhysics.connect([&]() {
            simulation.update();
        }));
    }

    void Handler::trackSimControlPanel(Simulation& simulation) {
        track(AppSignals::UI::StepPhysics.connect([&]() {
            simulation.update();
        }));
    }

    Handler::Handler(sf::RenderWindow& window, sf::View& sceneView, Simulation& simulation, std::unique_ptr<IRenderer>& renderer,
                     UiState& uiState) {
        trackIOPanel(window, uiState, simulation, renderer);
        trackToolsPanel(simulation, renderer, window, sceneView);
        trackSettingsPanel(window);
        trackSimControlPanel(simulation);
        trackKeyboard(simulation);
    }
}
