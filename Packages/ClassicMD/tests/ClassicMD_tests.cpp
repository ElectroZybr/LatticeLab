// Ordinary MD classes without NodeSystem, builders or plugin loading.
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <ClassicMD/AtomData.hpp>
#include <ClassicMD/AtomStorage.hpp>
#include <ClassicMD/ClassicMD.hpp>
#include <ClassicMD/SimulationLoop.hpp>
#include <Integrators/Verlet.hpp>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void simulationStepsThroughIntegrator() {
    ClassicMD::AtomStorage atoms;
    Integrators::Verlet verlet{atoms};
    verlet.dt = 0.1f;
    ClassicMD::ClassicMD simulation{atoms, verlet};
    check(atoms.size() == 1 && atoms.has("Energy") && atoms.has("PrevForceX"), "atoms and columns");
    simulation.step();
    check(atoms.get<Pos>(0).y > 0.0f, "simulation moved the atom");
}

void loopStopsPromptly() {
    ClassicMD::AtomStorage atoms;
    Integrators::Verlet verlet{atoms};
    ClassicMD::ClassicMD simulation{atoms, verlet};
    ClassicMD::SimulationLoop loop{simulation, std::chrono::hours(1)};
    std::thread worker([&] { loop.run(); });
    while (loop.steps() == 0)
        std::this_thread::yield();
    const auto started = std::chrono::steady_clock::now();
    loop.requestStop();
    worker.join();
    check(std::chrono::steady_clock::now() - started < std::chrono::seconds(1), "stop wakes the loop");
    check(loop.steps() == 1, "one step per period");
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
    loopStopsPromptly();
    atomDataHasNamedRows();
    std::cout << "ClassicMD: simulation, loop and atom data passed\n";
}
