#pragma once

#include "Benchmarks/BenchmarkCase.h"

namespace Lattice {
    class Simulation;
}

namespace Benchmarks {
    class BenchmarkScenes {
    public:
        static void build(Lattice::Simulation& simulation, const BenchmarkCase& benchmarkCase);

    private:
        static void buildIdealCrystal3D(Lattice::Simulation& simulation, const BenchmarkCase& benchmarkCase);
        static void buildCrystal2D(Lattice::Simulation& simulation, const BenchmarkCase& benchmarkCase);
        static void buildCrystal3D(Lattice::Simulation& simulation, const BenchmarkCase& benchmarkCase);
        static void buildRandomGas2D(Lattice::Simulation& simulation, const BenchmarkCase& benchmarkCase);
    };
}
