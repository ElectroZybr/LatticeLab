#include <benchmark/benchmark.h>

#include "BenchmarkCase.h"
#include "BenchmarkScenes.h"

#include "Engine/Simulation.h"
#include "Engine/physics/integrators/StepOps.h"
#include "Engine/physics/integrators/VerletScheme.h"
#include "Rendering/2d/Renderer2D.h"

#include "imgui.h"

namespace {

constexpr double kDt = 0.01;

Benchmarks::BenchmarkCase makeCrystal3DCase(int atomCount) {
    return Benchmarks::BenchmarkCase{
        .scene = Benchmarks::SceneKind::Crystal3D,
        .integrator = Integrator::Scheme::Verlet,
        .atomCount = atomCount,
        .boxStart = Vec3D(-80.0, -80.0, -80.0),
        .boxEnd = Vec3D(80.0, 80.0, 80.0),
        .cellSize = 5
    };
}

void rebuildScene(Simulation& simulation, int atomCount) {
    Benchmarks::BenchmarkScenes::build(simulation, makeCrystal3DCase(atomCount));
}

void prepareForPredict(Simulation& simulation, int atomCount) {
    rebuildScene(simulation, atomCount);
    StepOps::computeForces(simulation.atoms, simulation.sim_box, simulation.forceField, kDt);
}

void prepareForCorrect(Simulation& simulation, int atomCount) {
    prepareForPredict(simulation, atomCount);
    StepOps::predictAndSync(simulation.atoms, simulation.sim_box, kDt, &VerletScheme::predict);
    StepOps::computeForces(simulation.atoms, simulation.sim_box, simulation.forceField, kDt);
}

void setBenchmarkCounters(benchmark::State& state, int atomCount) {
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(atomCount));
    state.counters["atoms"] = static_cast<double>(atomCount);
    state.counters["ns_per_atom"] = benchmark::Counter(
        static_cast<double>(atomCount),
        benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert
    );
}

std::vector<Atom> makeRandomAtoms(int count) {
    std::vector<Atom> atoms;
    atoms.reserve(count);
    // равномерно распределяем по боксу
    const int side = static_cast<int>(std::cbrt(count)) + 1;
    for (int i = 0; i < count; ++i) {
        atoms.emplace_back(Vec3D((i % side) * 3.0, ((i / side) % side) * 3.0, (i / (side * side)) * 3.0),
            Vec3D::Random() * 0.5, Atom::Type::H
        );
    }
    return atoms;
}

}

static void BM_SimulationStep(benchmark::State& state) {
    const int atomCount = static_cast<int>(state.range(0));

    SimBox simBox(Vec3D(-30, -30, -30), Vec3D(30, 30, 30));
    Simulation simulation(simBox);

    for (auto _ : state) {
        state.PauseTiming();
        rebuildScene(simulation, atomCount);
        state.ResumeTiming();

        simulation.update(kDt);
        benchmark::ClobberMemory();
    }

    setBenchmarkCounters(state, atomCount);
}

static void BM_ComputeForces(benchmark::State& state) {
    const int atomCount = static_cast<int>(state.range(0));

    SimBox simBox(Vec3D(-30, -30, -30), Vec3D(30, 30, 30));
    Simulation simulation(simBox);

    for (auto _ : state) {
        state.PauseTiming();
        rebuildScene(simulation, atomCount);
        state.ResumeTiming();

        StepOps::computeForces(simulation.atoms, simulation.sim_box, simulation.forceField, kDt);
        benchmark::DoNotOptimize(simulation.atoms.data());
        benchmark::ClobberMemory();
    }

    setBenchmarkCounters(state, atomCount);
}

static void BM_PredictAndSync(benchmark::State& state) {
    const int atomCount = static_cast<int>(state.range(0));

    SimBox simBox(Vec3D(-30, -30, -30), Vec3D(30, 30, 30));
    Simulation simulation(simBox);

    for (auto _ : state) {
        state.PauseTiming();
        prepareForPredict(simulation, atomCount);
        state.ResumeTiming();

        StepOps::predictAndSync(simulation.atoms, simulation.sim_box, kDt, &VerletScheme::predict);
        benchmark::DoNotOptimize(simulation.atoms.data());
        benchmark::ClobberMemory();
    }

    setBenchmarkCounters(state, atomCount);
}

static void BM_Correct(benchmark::State& state) {
    const int atomCount = static_cast<int>(state.range(0));

    SimBox simBox(Vec3D(-30, -30, -30), Vec3D(30, 30, 30));
    Simulation simulation(simBox);

    for (auto _ : state) {
        state.PauseTiming();
        prepareForCorrect(simulation, atomCount);
        state.ResumeTiming();

        for (Atom& atom : simulation.atoms) {
            if (!atom.isFixed) {
                VerletScheme::correct(atom, kDt);
            }
        }

        benchmark::DoNotOptimize(simulation.atoms.data());
        benchmark::ClobberMemory();
    }

    setBenchmarkCounters(state, atomCount);
}

static void BM_Renderer2D_DrawShot(benchmark::State& state) {
    sf::RenderTexture renderTexture;
    if (!renderTexture.resize({800, 600})) {
        state.SkipWithError("RenderTexture resize failed");
        return;
    }

    sf::View gameView(sf::FloatRect({0, 0}, {800, 600}));
    sf::View uiView(sf::FloatRect({0, 0}, {800, 600}));
    Renderer2D renderer(renderTexture, gameView, uiView);

    std::vector<Atom> atoms = makeRandomAtoms(static_cast<int>(state.range(0)));

    SimBox box(Vec3D(0, 0, 0), Vec3D(300, 300, 300));

    for (auto _ : state) {
        renderer.drawShot(atoms, box, 0.016f);
        benchmark::ClobberMemory();
    }
}


BENCHMARK(BM_SimulationStep)->Args({5 * 5 * 5})->Args({10 * 10 * 10})->Args({20 * 20 * 20});
BENCHMARK(BM_ComputeForces)->Args({5 * 5 * 5})->Args({10 * 10 * 10})->Args({20 * 20 * 20});
BENCHMARK(BM_PredictAndSync)->Args({5 * 5 * 5})->Args({10 * 10 * 10})->Args({20 * 20 * 20});
BENCHMARK(BM_Correct)->Args({5 * 5 * 5})->Args({10 * 10 * 10})->Args({20 * 20 * 20});
BENCHMARK(BM_Renderer2D_DrawShot)->Args({5 * 5 * 5})->Args({30 * 30 * 30})->Args({100 * 100 * 100});