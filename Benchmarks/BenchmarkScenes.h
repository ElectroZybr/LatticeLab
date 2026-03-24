#pragma once

#include "BenchmarkCase.h"

class Simulation;

namespace Benchmarks {
    class BenchmarkScenes {
    public:
        static void build(Simulation& simulation, const BenchmarkCase& benchmarkCase);

    private:
        static void buildCrystal2D(Simulation& simulation, const BenchmarkCase& benchmarkCase);
        static void buildCrystal3D(Simulation& simulation, const BenchmarkCase& benchmarkCase);
        static void buildRandomGas2D(Simulation& simulation, const BenchmarkCase& benchmarkCase);
    };
}
