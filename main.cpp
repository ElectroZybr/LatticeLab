#include <cmath>
#include <string_view>

#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowHandle.hpp>

#include "imgui-SFML.h"

#include "GUI/io/manager/EventManager.h"
#include "GUI/io/keyboard/Keyboard.h"

#include "Engine/Simulation.h"

#include "GUI/interface/interface.h"
#include "GUI/interface/panels/debug/view/DebugView.h"
#include "GUI/interface/panels/debug/DebugDrawers.h"
#include "Engine/utils/Timer.h"

#include "Rendering/2d/Renderer2D.h"
#include "Rendering/3d/Renderer3D.h"

#include "Engine/Tools.h"

#include "memory_monitor.h"

constexpr int FPS = 60;
constexpr int LPS = 20;
constexpr double Dt = 0.01;

std::string_view schemeName(Integrator::Scheme s) {
    switch (s) {
        case Integrator::Scheme::Verlet:   return "Velocity Verlet";
        case Integrator::Scheme::KDK:      return "KDK (Kick-Drift-Kick)";
        case Integrator::Scheme::RK4:      return "Runge-Kutta 4";
        case Integrator::Scheme::Langevin: return "Langevin";
    }
    return "Unknown";
}

namespace Scenes {
    void crystal(Simulation& sim, int n, Atom::Type type, bool is3d,
                 double padding = 3.0, double margin = 15.0)
    {
        const double side = n * padding + padding + 2.0 * margin;
        const double half = side / 2.0;
 
        sim.setSizeBox(
            Vec3D(-half, -half, is3d ? -half : sim.sim_box.start.z),
            Vec3D( half,  half, is3d ?  half : sim.sim_box.end.z)
        );
 
        const Vec3D vecMargin(margin, margin, is3d ? margin : 0.0);
        const int   zMax = is3d ? n : 1;
 
        for (int x = 1; x <= n; ++x)
            for (int y = 1; y <= n; ++y)
                for (int z = 1; z <= zMax; ++z)
                    sim.createAtom(Vec3D(x, y, z) * padding + vecMargin,
                                   Vec3D::Random() * 0.5, type);
    }
 
    void diffusionTest(Simulation& sim)
    {
        sim.setSizeBox(
            Vec3D(-25, -25, sim.sim_box.start.z),
            Vec3D( 25,  25, sim.sim_box.end.z),
            5
        );
 
        for (int i = 0; i < 15; ++i) {
            for (int j = 0;  j < 8;  ++j)
                sim.createAtom(Vec3D(4 + i*3, 4 + j*3, 1), Vec3D::Random() * 0.5, Atom::Type::H);
            for (int j = 8;  j < 15; ++j)
                sim.createAtom(Vec3D(4 + i*3, 4 + j*3, 1), Vec3D::Random() * 0.5, Atom::Type::O);
        }
    }
}

static sf::RenderWindow createWindow() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
 
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Chemical-simulator",
                            sf::State::Fullscreen, settings);
 
    sf::Image icon;
    if (icon.loadFromFile("assets/icon.png"))
        window.setIcon(icon.getSize(), icon.getPixelsPtr());
 
    return window;
}

static DebugView* buildDebugSimView(DebugPanel& panel) {
    return panel.addView(DebugView("Симуляция", {
        DebugValue ("Память (МБ)"),
        DebugValue ("Рендер (мс)"),
        DebugValue ("Физика (мс)"),
        DebugValue ("Тип интегратора", DebugDrawers::String),
        DebugValue ("Шаги симуляции", DebugDrawers::Int),
        DebugValue ("Количество атомов", DebugDrawers::Int),
        DebugSeries("Полная энергия"),
    }));
}

static DebugView* buildDebugAtomSingle(DebugPanel& panel) {
    return panel.addView(DebugView("Атом", {
        DebugValue("Позиция", DebugDrawers::Vec3D),
        DebugValue("Скорость", DebugDrawers::Vec3D),
        DebugValue("Силы", DebugDrawers::Vec3D),
        DebugValue("Пред. силы", DebugDrawers::Vec3D),
        DebugValue("Потенциальная энергия", DebugDrawers::Float),
        DebugValue("Масса"),
        DebugValue("Радиус"),
        DebugValue("Тип", DebugDrawers::Int),
    }));
}

static DebugView* buildDebugAtomBatch(DebugPanel& panel) {
    return panel.addView(DebugView("Атомы", {
        DebugValue("Выбрано атомов", DebugDrawers::Int),
    }));
}

struct RateCounter {
    Timer  timer;
    double accumulated_ms  = 0.0;
    int    steps_this_tick  = 0;
    float  steps_per_second = 0.0f;
 
    void startStep()  { timer.start(); }
 
    void finishStep() {
        timer.stop();
        accumulated_ms += timer.elapsedMilliseconds();
        ++steps_this_tick;
    }
 
    float avgMs() const {
        return steps_this_tick > 0
            ? static_cast<float>(accumulated_ms / steps_this_tick)
            : 0.0f;
    }
 
    void flush(double elapsed_seconds) {
        steps_per_second = static_cast<float>(steps_this_tick / elapsed_seconds);
        accumulated_ms   = 0.0;
        steps_this_tick  = 0;
    }
};

int main() {
    sf::RenderWindow window = createWindow();
    sf::View& gameView = const_cast<sf::View&>(window.getView());

    // Симуляция
    SimBox box(Vec3D(-25, -25, 0), Vec3D(25, 25, 6));
    Simulation simulation(box);
    simulation.setIntegrator(Integrator::Scheme::Verlet);
    Scenes::crystal(simulation, 20, Atom::Type::Z, false);

    // Рендер
    std::unique_ptr<IRenderer> renderer = std::make_unique<Renderer2D>(window, gameView);
    renderer->drawBonds = true;
    renderer->speedGradient = true;

    // Интерфейс
    Interface::init(window, simulation, renderer);
    EventManager::init(&window, &gameView, renderer, &simulation.sim_box, &simulation.atoms);
    Tools::init(&window, &gameView, &box.grid, &box, renderer,
        [&](Vec3D coords, Vec3D speed, Atom::Type type, bool fixed) {
            return simulation.createAtom(coords, speed, type, fixed);
        }
    );
    Interface::pause = true;

    // Дебаг-панель
    DebugView* debugSim = buildDebugSimView(Interface::debugPanel);
    DebugView* debugAtomSingle = buildDebugAtomSingle(Interface::debugPanel);
    DebugView* debugAtomBatch = buildDebugAtomBatch(Interface::debugPanel);

    // Главный цикл
    sf::Clock clock;
    RateCounter physicsCounter;
    RateCounter renderCounter;

    double renderAccum = 0.0;
    double physicsAccum = 0.0;
    double logAccum = 0.0;

    constexpr double RENDER_INTERVAL = 1.0 / FPS;
    constexpr double LOG_INTERVAL    = 1.0 / LPS;

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        physicsAccum += deltaTime;
        renderAccum += deltaTime;
        logAccum += deltaTime;

        EventManager::poll();
        EventManager::frame(deltaTime);

        if (!Interface::getPause()) {
            const double physicsInterval = 1.0 / Interface::getSimulationSpeed();
            physicsCounter.startStep();
            while (physicsAccum >= physicsInterval) {
                simulation.update(Dt);
                physicsAccum -= physicsInterval;
            }
            physicsCounter.finishStep();
        }
        else {
            physicsAccum = 0.0;
        }

        if (auto cmd = Keyboard::popResult(); cmd == KeyboardCommand::StepPhysics) {
            physicsCounter.startStep();
            simulation.update(Dt);
            physicsCounter.finishStep();
        }

        if (renderAccum >= RENDER_INTERVAL) {
            renderAccum -= RENDER_INTERVAL;

            Interface::Update();

            if (Tools::selected_atom_batch.size() == 1)
            {
                debugAtomSingle->visible = true;
                debugAtomBatch->visible = false;
                const Atom* selectedAtom = *Tools::selected_atom_batch.begin();
                debugAtomSingle->add_data("Позиция", selectedAtom->coords);
                debugAtomSingle->add_data("Скорость", selectedAtom->speed);
                debugAtomSingle->add_data("Силы", selectedAtom->force);
                debugAtomSingle->add_data("Пред. силы", selectedAtom->prev_force);
                debugAtomSingle->add_data("Потенциальная энергия", selectedAtom->potential_energy);
                debugAtomSingle->add_data("Масса", Atom::getProps(selectedAtom->type).mass);
                debugAtomSingle->add_data("Радиус", Atom::getProps(selectedAtom->type).radius);
                debugAtomSingle->add_data("Тип", static_cast<int>(selectedAtom->type));
            }
            else {
                debugAtomBatch->visible = true;
                debugAtomSingle->visible = false;
                debugAtomBatch->add_data("Выбрано атомов", Tools::selected_atom_batch.size());
            }

            // Диалог сохранения / загрузки
            if (auto result = Interface::fileDialog.popResult()) {
                switch (result->command) {
                    case FileDialogCommand::Save: simulation.save(result->path); break;
                    case FileDialogCommand::Load: simulation.load(result->path); break;
                }
            }

            // Смена рендерера
            if (auto result = Interface::toolsPanel.popResult()) {
                std::unique_ptr<IRenderer> newRenderer;
                switch (result.value()) {
                    case ToolsCommand::ToggleRenderer2D:
                        newRenderer = std::make_unique<Renderer2D>(window, gameView);
                        break;
                    case ToolsCommand::ToggleRenderer3D:
                        newRenderer = std::make_unique<Renderer3D>(window, gameView);
                        break;
                }

                newRenderer->drawGrid = renderer->drawGrid;
                newRenderer->drawBonds = renderer->drawBonds;
                newRenderer->speedGradient = renderer->speedGradient;
                newRenderer->speedGradientTurbo = renderer->speedGradientTurbo;

                renderer = std::move(newRenderer);
            }

            renderer->camera.update(window);

            renderCounter.startStep();
            renderer->drawShot(simulation.atoms, simulation.sim_box);
            ImGui::SFML::Render(window);
            window.display();
            renderCounter.finishStep();
        }

        if (logAccum >= LOG_INTERVAL) {
            logAccum -= LOG_INTERVAL;
            debugSim->add_data("Полная энергия", static_cast<float>(simulation.fullAverageEnergy()));
            debugSim->add_data("Память (МБ)", MemoryMonitor::getRSS() / 1024.f / 1024.f);
            debugSim->add_data("Рендер (мс)", renderCounter.avgMs());
            debugSim->add_data("Физика (мс)", physicsCounter.avgMs());
            debugSim->add_data("Количество атомов", simulation.atoms.size());
            debugSim->add_data("Шаги симуляции", simulation.getSimStep());
            debugSim->add_data("Тип интегратора", schemeName(simulation.getIntegrator()));

            physicsCounter.flush(logAccum);
            renderCounter.flush(logAccum);
        }
    }
    ImGui::SFML::Shutdown();

    return 0;
}
