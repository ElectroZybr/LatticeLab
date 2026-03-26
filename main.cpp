#include "app/Application.h"

int main() {
<<<<<<< HEAD
    Application application;
    return application.run();
=======
    sf::RenderWindow window = createWindow();
    if (!window.isOpen()) return EXIT_FAILURE;
    sf::View& gameView = const_cast<sf::View&>(window.getView());

    // Симуляция
    SimBox box(Vec3f(-25, -25, 0), Vec3f(25, 25, 6));
    Simulation simulation(box);
    simulation.setIntegrator(Integrator::Scheme::Verlet);
    Scenes::crystal(simulation, 20, AtomData::Type::Z, false);

    // Рендер
    std::unique_ptr<IRenderer> renderer = std::make_unique<Renderer2D>(window, gameView);
    renderer->setAtomStorage(&simulation.atomStorage);
    renderer->drawBonds = true;
    renderer->speedGradient = true;

    // Интерфейс
    Interface::init(window, simulation, renderer);
    EventManager::init(&window, &gameView, renderer, &simulation.sim_box, &simulation.atomStorage);
    Tools::init(&window, &gameView, &box.grid, &box, renderer, &simulation.atomStorage,
        [&](Vec3f coords, Vec3f speed, AtomData::Type type, bool fixed) {
            return simulation.createAtom(coords, speed, type, fixed);
        },
        [&](std::size_t atomIndex) {
            return simulation.removeAtom(atomIndex);
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
            if (physicsAccum >= physicsInterval) {
                physicsCounter.startStep();
                simulation.update(Dt);
                physicsCounter.finishStep();
                physicsAccum = 0.0;
            }
        } else {
            physicsAccum = 0.0;
        }

        // один шаг симуляции
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
                const std::size_t selectedIndex = *Tools::selected_atom_batch.begin();
                if (selectedIndex < simulation.atomStorage.size()) {
                    debugAtomSingle->add_data("Позиция", simulation.atomStorage.pos(selectedIndex));
                    debugAtomSingle->add_data("Скорость", simulation.atomStorage.vel(selectedIndex));
                    debugAtomSingle->add_data("Силы", simulation.atomStorage.force(selectedIndex));
                    debugAtomSingle->add_data("Пред. силы", simulation.atomStorage.prevForce(selectedIndex));
                    debugAtomSingle->add_data("Потенциальная энергия", simulation.atomStorage.energy(selectedIndex));
                    const AtomData::Type atomType = simulation.atomStorage.type(selectedIndex);
                    debugAtomSingle->add_data("Масса", AtomData::getProps(atomType).mass);
                    debugAtomSingle->add_data("Радиус", AtomData::getProps(atomType).radius);
                    debugAtomSingle->add_data("Тип", static_cast<int>(atomType));
                }
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
                    case FileDialogCommand::Load:
                        simulation.load(result->path);
                        Tools::resetInteractionState();
                        break;
                }
            }

            // Смена рендерера
            if (auto result = Interface::toolsPanel.popResult()) {
                if (result.value() == ToolsCommand::ClearSimulation) {
                    simulation.clear();
                    Tools::resetInteractionState();
                }
                else {
                std::unique_ptr<IRenderer> newRenderer;
                switch (result.value()) {
                    case ToolsCommand::ToggleRenderer2D:
                        newRenderer = std::make_unique<Renderer2D>(window, gameView);
                        break;
                    case ToolsCommand::ToggleRenderer3D:
                        newRenderer = std::make_unique<Renderer3D>(window, gameView);
                        break;
                    case ToolsCommand::ClearSimulation:
                        break;
                }

                if (newRenderer) {
                    newRenderer->drawGrid = renderer->drawGrid;
                    newRenderer->drawBonds = renderer->drawBonds;
                    newRenderer->speedGradient = renderer->speedGradient;
                    newRenderer->speedGradientTurbo = renderer->speedGradientTurbo;
                    newRenderer->speedGradientMax = renderer->speedGradientMax;
                    newRenderer->setAtomStorage(&simulation.atomStorage);

                    renderer = std::move(newRenderer);
                }
                }
            }

            renderer->camera.update(window);

            renderCounter.startStep();
            renderer->drawShot(simulation.atomStorage, simulation.sim_box);
            renderer->drawOverlay(OverlayState());
            ImGui::SFML::Render(window);
            window.display();
            renderCounter.finishStep();
        }

        // обновение логов и данных счетчиков
        if (logAccum >= LOG_INTERVAL) {
            logAccum -= LOG_INTERVAL;
            debugSim->add_data("Полная энергия", static_cast<float>(simulation.fullAverageEnergy()));
            debugSim->add_data("Память (МБ)", MemoryMonitor::getRSS() / 1024.f / 1024.f);
            debugSim->add_data("Рендер (мс)", renderCounter.avgMs());
            debugSim->add_data("Физика (мс)", physicsCounter.avgMs());
            debugSim->add_data("Количество атомов", simulation.atomStorage.size());
            debugSim->add_data("Шаги симуляции", simulation.getSimStep());
            debugSim->add_data("Шагов/с", physicsCounter.steps_per_second);
            debugSim->add_data("Тип интегратора", schemeName(simulation.getIntegrator()));

            physicsCounter.flush(LOG_INTERVAL);
            renderCounter.flush(LOG_INTERVAL);
        }
    }
    Interface::shutdown();

    return 0;
>>>>>>> d08f622 (feat: создан класс PickingSystem и OverlayState)
}
