#include <Lattice/Composition.hpp>
#include <latticebench/Benchmarks.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "Bench",
    .version = {1, 0, 0},
};

extern "C" void compose(NodeBuild root) {
    auto bench = root.emplace<Benchmarks>("bench");
    bench.action("run", &Benchmarks::run);
    bench.action("list", &Benchmarks::list);
    bench.action("view", &Benchmarks::view);
}
