// Hot paths of the boundaries library: run `xmake run Boundaries.bench [name]`.
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include <BmRunner/Benchmarks.hpp>
#include <Boundaries/Space.hpp>
#include <BoundariesCpu/Geometry.hpp>
#include <BoundariesCpu/Walls.hpp>
#include <BoundariesCpu/Wrap.hpp>
#include <BoundariesWgpu/GpuGeometry.hpp>
#include <BoundariesWgpu/GpuWalls.hpp>
#include <Wgpu/CommandList.hpp>
#include <Wgpu/Device.hpp>
#include <Wgpu/GpuSoA.hpp>
#include <Lattice/Tools/TypeName.hpp>

using namespace Boundaries;
using namespace Boundaries::Cpu;

namespace {

struct PX { using type = float; };
struct PY { using type = float; };
struct PZ { using type = float; };
struct VX { using type = float; };
struct VY { using type = float; };
struct VZ { using type = float; };
struct FX { using type = float; };
struct FY { using type = float; };
struct FZ { using type = float; };

const ParticleLayout Layout{
    std::string(Lattice::typeName<PX>()), std::string(Lattice::typeName<PY>()), std::string(Lattice::typeName<PZ>()),
    std::string(Lattice::typeName<VX>()), std::string(Lattice::typeName<VY>()), std::string(Lattice::typeName<VZ>()),
    std::string(Lattice::typeName<FX>()), std::string(Lattice::typeName<FY>()), std::string(Lattice::typeName<FZ>())};

/// Particle table as a simulation keeps it, plus its initial state for prepare().
struct Table {
    explicit Table(size_t n) {
        for (auto* t : {&table, &initial}) {
            t->addCol<PX>(), t->addCol<PY>(), t->addCol<PZ>();
            t->addCol<VX>(), t->addCol<VY>(), t->addCol<VZ>();
            t->addCol<FX>(), t->addCol<FY>(), t->addCol<FZ>();
            t->resize(n);
        }
    }
    /// Restore the initial state (one block copy).
    void reset() {
        auto from = initial.bytes();
        std::copy(from.begin(), from.end(), const_cast<std::byte*>(table.bytes().data()));
    }
    StdData::SoA table, initial;
};

const std::vector<size_t> Sizes = {1'000, 10'000, 100'000, 1'000'000};

/// Points spread over a box larger than the space: wraps and walls both happen.
struct Points : Lattice::Fixture {
    explicit Points(size_t n) : x(n), y(n), z(n), out(n) {
        std::mt19937 random(1);
        std::uniform_real_distribution<float> coordinate(-15, 25);
        for (size_t i = 0; i < n; ++i)
            sx.push_back(coordinate(random)), sy.push_back(coordinate(random)), sz.push_back(coordinate(random));
    }
    void prepare() override { x = sx, y = sy, z = sz; }

    std::vector<float> x, y, z, out;
    std::vector<float> sx, sy, sz;
};

/// A test tube with a mixer: capsule region minus a blended paddle, rounded.
Shape tube() {
    auto vessel = Shape::capsule({5, 1, 5}, {5, 9, 5}, 4);
    auto shaft = Shape::capsule({5, 3, 5}, {5, 10, 5}, 0.3f);
    auto paddle = std::move(Shape::box({2, 0.3f, 0.2f})).translated({5, 3, 5});
    return std::move(vessel) - std::move(Shape::smoothUnion(std::move(shaft), std::move(paddle), 0.2f)).rounded(0.05f);
}

}

BENCH_GROUP(Space) { bench.config.sizes = Sizes; }

namespace {
/// Positions spread over three periods of the box.
struct WrapTable : Lattice::Fixture, Table {
    explicit WrapTable(size_t n) : Table(n) {
        std::mt19937 random(1);
        std::uniform_real_distribution<float> coordinate(-15, 25);
        auto x = initial.span<PX>(), y = initial.span<PY>(), z = initial.span<PZ>();
        for (size_t i = 0; i < n; ++i)
            x[i] = coordinate(random), y[i] = coordinate(random), z[i] = coordinate(random);
    }
    void prepare() override { reset(); }
};
}

BENCH_GROUPED(Space, WrapColumns, WrapTable, "wrap positions of a SoA, three periodic axes") {
    const Space space{{0, 0, 0}, {10, 10, 10}, {true, true, true}};
    bench.measure<WrapTable>([&](WrapTable& t) { wrapPositions(space, t.table, Layout, t.table.size()); });
}

BENCH_GROUPED(Space, MinimumImage, Points, "minimum image of consecutive separations") {
    const Space space{{0, 0, 0}, {10, 10, 10}, {true, true, true}};
    bench.measure<Points>([&](Points& points) {
        float sum = 0;
        for (size_t i = 1; i < points.x.size(); ++i)
            sum += space.minimumImage({points.x[i] - points.x[i - 1], points.y[i] - points.y[i - 1],
                points.z[i] - points.z[i - 1]}).x;
        return sum;
    });
}

BENCH_GROUP(Geometry) { bench.config.sizes = Sizes; }

BENCH_GROUPED(Geometry, EvaluateBlocks, Points, "tube with a mixer, SoA in blocks of 64") {
    const Geometry geometry{tube()};
    bench.measure<Points>([&](Points& points) { geometry.evaluate(points.x, points.y, points.z, points.out); });
}

BENCH_GROUPED(Geometry, SamplePoints, Points, "same program, one point at a time") {
    const Geometry geometry{tube()};
    bench.measure<Points>([&](Points& points) {
        float sum = 0;
        for (size_t i = 0; i < points.x.size(); ++i)
            sum += geometry.distance({points.x[i], points.y[i], points.z[i]});
        return sum;
    });
}

BENCH_GROUPED(Geometry, Normals, Points, "normal per point (four evaluations)") {
    const Geometry geometry{tube()};
    bench.measure<Points>([&](Points& points) {
        float sum = 0;
        for (size_t i = 0; i < points.x.size(); ++i)
            sum += geometry.normal({points.x[i], points.y[i], points.z[i]}).x;
        return sum;
    });
}

namespace {

/// Particles inside and around the tube: most in the region, some in the walls.
struct TubeParticles : Lattice::Fixture, Table {
    explicit TubeParticles(size_t n) : Table(n) {
        std::mt19937 random(2);
        std::uniform_real_distribution<float> radial(0, 4.4f), angle(0, 6.2831853f), height(0, 10), speed(-1, 1);
        auto x = initial.span<PX>(), y = initial.span<PY>(), z = initial.span<PZ>();
        auto vx = initial.span<VX>(), vy = initial.span<VY>(), vz = initial.span<VZ>();
        for (size_t i = 0; i < n; ++i) {
            const float r = radial(random), a = angle(random);
            x[i] = 5 + r * std::cos(a), y[i] = height(random), z[i] = 5 + r * std::sin(a);
            vx[i] = speed(random), vy[i] = speed(random), vz[i] = speed(random);
        }
    }
    void prepare() override { reset(); }
};

}

BENCH_GROUP(Walls) { bench.config.sizes = Sizes; }

BENCH_GROUPED(Walls, Constrain, TubeParticles, "hard tube walls: project and reflect the ~10% in walls") {
    const Geometry geometry{tube()};
    const Walls walls{geometry, Layout};
    bench.measure<TubeParticles>([&](TubeParticles& p) { walls.constrain(p.table, p.table.size()); });
}

BENCH_GROUPED(Walls, SoftForces, TubeParticles, "soft tube walls, layer 0.5 thick") {
    const Geometry geometry{tube()};
    const Walls walls{geometry, Layout, {Laws::Soft::make(10, 0.5f, 6)}};
    bench.measure<TubeParticles>([&](TubeParticles& p) { walls.addForces(p.table, p.table.size()); });
}

namespace {

::Wgpu::Device& device() {
    static ::Wgpu::Instance instance;
    static ::Wgpu::Device device{instance};
    return device;
}

/// The tube particles resident on the GPU; prepare() uploads the initial state again.
struct GpuTubeParticles : TubeParticles {
    explicit GpuTubeParticles(size_t n) : TubeParticles(n), gpu(device(), "bench/particles") { prepare(); }
    void prepare() override {
        TubeParticles::prepare();
        gpu.upload(table);
        device().wait();
    }
    ::Wgpu::GpuSoA gpu;
};

}

BENCH_GROUPED(Walls, ConstrainGpu, GpuTubeParticles, "hard tube walls on the GPU: record, submit, wait") {
    const Boundaries::Wgpu::GpuGeometry geometry{device(), tube().compile()};
    Boundaries::Wgpu::GpuWalls walls{geometry, Layout};
    bench.measure<GpuTubeParticles>([&](GpuTubeParticles& p) {
        ::Wgpu::CommandList commands{device()};
        walls.constrain(commands, p.gpu, p.table.size());
        commands.submit();
        device().wait();
    });
}

BENCH_GROUPED(Walls, SoftForcesGpu, GpuTubeParticles, "soft tube walls on the GPU: record, submit, wait") {
    const Boundaries::Wgpu::GpuGeometry geometry{device(), tube().compile()};
    Boundaries::Wgpu::GpuWalls walls{geometry, Layout, {Laws::Soft::make(10, 0.5f, 6)}};
    bench.measure<GpuTubeParticles>([&](GpuTubeParticles& p) {
        ::Wgpu::CommandList commands{device()};
        walls.addForces(commands, p.gpu, p.table.size());
        commands.submit();
        device().wait();
    });
}

int main(int argc, char** argv) {
    if (argc > 1)
        Lattice::Benchmarks::run(argv[1]);
    else
        Lattice::Benchmarks::runAll();
}
