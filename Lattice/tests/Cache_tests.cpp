#include <cmath>

#include <Lattice/Tools/BmRunner/Metrics/Cache.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {

TEST(Cache_ComputesMissesPerKiloInstructions, Fixture) {
    REQUIRE(
        std::abs(
            Benchmarks::Cache::missesPerKiloInstructions(25, 10'000) -
            2.5
        ) < 1e-12
    );

    REQUIRE(
        Benchmarks::Cache::missesPerKiloInstructions(25, 0) == 0.0
    );
}

}
