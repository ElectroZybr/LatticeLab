#include <benchmark/benchmark.h>

#include <cstddef>

#include "fixtures/SimulationFixture.h"

// @bench_meta {"id":"SimulationFixture/ForEachNeighbor","ru":"Обход соседних ячеек (forEachNeighbor)","group":"Симуляция/Сетка и соседи"}
BENCHMARK_DEFINE_F(SimulationFixture, ForEachNeighbor)(benchmark::State& state) {
    rebuildScene();

    const auto& atoms = simulation_->atomStorage;
    const auto& grid = simulation_->sim_box.grid;

    std::size_t visited = 0;
    std::size_t checksum = 0;

    for (auto _ : state) {
        visited = 0;
        checksum = 0;

        for (std::size_t atomIndex = 0; atomIndex < atoms.size(); ++atomIndex) {
            simulation_->neighborList.forEachNeighbor(grid, atoms, atomIndex, [&](std::size_t neighborIndex) {
                ++visited;
                checksum += neighborIndex;
            });
        }

        benchmark::DoNotOptimize(visited);
        benchmark::DoNotOptimize(checksum);
        benchmark::ClobberMemory();
    }

    state.counters["neighbors_visited"] = static_cast<double>(visited);
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, ForEachNeighbor)
    ->Args({125})   // 5^3
    ->Args({216})   // 6^3
    ->Args({343})   // 7^3
    ->Args({512})   // 8^3
    ->Args({729})   // 9^3
    ->Args({1000})  // 10^3
    ->Args({1728}); // 12^3

// @bench_meta {"id":"SimulationFixture/WriteAtomNeighbors","ru":"Запись соседей (writeAtomNeighbors)","group":"Симуляция/Сетка и соседи"}
BENCHMARK_DEFINE_F(SimulationFixture, WriteAtomNeighbors)(benchmark::State& state) {
    rebuildScene();

    const auto& atoms = simulation_->atomStorage;
    const auto& grid = simulation_->sim_box.grid;

    std::size_t visited = 0;
    std::size_t checksum = 0;
    std::vector<std::size_t> tmp;
    tmp.reserve(atoms.size() * 16);

    for (auto _ : state) {
        visited = 0;
        checksum = 0;
        tmp.clear();

        for (std::size_t atomIndex = 0; atomIndex < atoms.size(); ++atomIndex) {
            const std::size_t before = tmp.size();
            simulation_->neighborList.writeAtomNeighbors(grid, atoms, atomIndex, tmp);
            const std::size_t after = tmp.size();
            visited += (after - before);
            for (std::size_t idx = before; idx < after; ++idx) {
                checksum += tmp[idx];
            }
        }

        benchmark::DoNotOptimize(visited);
        benchmark::DoNotOptimize(checksum);
        benchmark::ClobberMemory();
    }

    state.counters["neighbors_visited"] = static_cast<double>(visited);
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, WriteAtomNeighbors)
    ->Args({125})   // 5^3
    ->Args({216})   // 6^3
    ->Args({343})   // 7^3
    ->Args({512})   // 8^3
    ->Args({729})   // 9^3
    ->Args({1000})  // 10^3
    ->Args({1728}); // 12^3
