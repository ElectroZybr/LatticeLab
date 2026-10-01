#include <Lattice/Tools/BmRunner/Metrics/PerfEvent.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {

TEST(PerfEvent_ScalesMultiplexedDelta, Fixture) {
    using Snapshot = Benchmarks::PerfEvent::Snapshot;

    REQUIRE(
        Benchmarks::PerfEvent::delta(
            Snapshot{
                .value = 100,
                .timeEnabled = 1'000,
                .timeRunning = 500
            },
            Snapshot{
                .value = 300,
                .timeEnabled = 1'200,
                .timeRunning = 550
            }
        ) == 800
    );

    REQUIRE(
        Benchmarks::PerfEvent::delta(
            Snapshot{.value = 100},
            Snapshot{.value = 300}
        ) == 200
    );

    REQUIRE(
        Benchmarks::PerfEvent::delta(
            Snapshot{
                .value = 100,
                .timeEnabled = 1'000,
                .timeRunning = 500
            },
            Snapshot{
                .value = 100,
                .timeEnabled = 1'200,
                .timeRunning = 500
            }
        ) == 0
    );
}

}
