// Ordinary MD classes without NodeSystem, builders or package loading.
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <ClassicMD/AtomData.hpp>
#include <ClassicMD/AtomStorage.hpp>
#include <ClassicMD/ClassicMD.hpp>
#include <Integrators/Verlet.hpp>
#include <Tasks/Queue.hpp>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void simulationStepsThroughIntegrator() {
    ClassicMD::AtomStorage atoms;
    Integrators::Verlet verlet{atoms};
    verlet.dt = 0.1f;
    Tasks::Queue commands;
    ClassicMD::ClassicMD simulation{atoms, verlet, commands};
    check(atoms.size() == 1 && atoms.has("Energy") && atoms.has("PrevForceX"), "atoms and columns");
    simulation.step();
    check(atoms.get<Pos>(0).y > 0.0f, "simulation moved the atom");
}

void commandsRunOnTheSimulationThread() {
    ClassicMD::AtomStorage atoms;
    Integrators::Verlet verlet{atoms};
    Tasks::Queue commands;
    ClassicMD::ClassicMD simulation{atoms, verlet, commands, std::chrono::hours(1)};
    simulation.commands().post([&] { verlet.dt = 0.5f; });
    check(verlet.dt == 0.5f, "without a running loop a command runs at once");

    std::thread worker([&] { simulation.run(); });
    while (!simulation.commands().serving())
        std::this_thread::yield();
    const auto thread = simulation.commands().call([] { return std::this_thread::get_id(); });
    check(thread == worker.get_id(), "commands run on the simulation thread");
    const auto before = simulation.commands().call([&] { return simulation.steps(); });
    simulation.commands().call([&] { simulation.step(); return 0; });
    check(simulation.commands().call([&] { return simulation.steps(); }) == before + 1,
          "an extra step runs between the loop's steps");
    simulation.requestStop();
    worker.join();
}

void loopStopsPromptly() {
    ClassicMD::AtomStorage atoms;
    Integrators::Verlet verlet{atoms};
    Tasks::Queue commands;
    ClassicMD::ClassicMD simulation{atoms, verlet, commands, std::chrono::hours(1)};
    std::thread worker([&] { simulation.run(); });
    while (simulation.steps() == 0)
        std::this_thread::yield();
    const auto started = std::chrono::steady_clock::now();
    simulation.requestStop();
    worker.join();
    check(std::chrono::steady_clock::now() - started < std::chrono::seconds(1), "stop wakes the loop");
    check(simulation.steps() == 1, "one step per period");
}

void atomDataHasNamedRows() {
    ClassicMD::AtomData data;
    const size_t h = data.addRow("H");
    data.at<ClassicMD::Mass>(h) = 1.008f;
    check(data.requireRow("H") == h && data.columns() == 3, "atom data");
}
}

int main() {
    simulationStepsThroughIntegrator();
    commandsRunOnTheSimulationThread();
    loopStopsPromptly();
    atomDataHasNamedRows();
    std::cout << "ClassicMD: simulation, commands, loop and atom data passed\n";
}
