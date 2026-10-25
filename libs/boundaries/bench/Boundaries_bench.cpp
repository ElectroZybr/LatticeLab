// Hot paths of the boundaries library: run `xmake run Boundaries.bench [name or group]`.
// Walls: implementations of one work side by side — cpu (one thread and all of them, the
// Tasks::Workers axis) and gpu, each checked against cpu before it is measured — split into
// phases (BmRunner Trace); ConstrainOffload measures data coming from memory (cold cache /
// copies there and back). A group run prints the crossovers N*.
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include <lbm-runner/Benchmarks.hpp>
#include <lbm-runner/Main.hpp>
#include <lbm-runner/Evict.hpp>
#include <boundaries/Space.hpp>
#include <boundariescpu/Geometry.hpp>
#include <boundariescpu/Walls.hpp>
#include <boundariescpu/Wrap.hpp>
#include <boundarieswgpu/GpuGeometry.hpp>
#include <boundarieswgpu/GpuWalls.hpp>
#include <wgpu/CommandList.hpp>
#include <wgpu/Device.hpp>
#include <wgpu/GpuSoA.hpp>
#include <wgpu/Timer.hpp>
#include <tasks/Workers.hpp>
#include <Lattice/tools/TypeName.hpp>

using namespace Boundaries;
using namespace Boundaries::Cpu;
using Lattice::Benchmarks::Phase;
using Lattice::Benchmarks::Run;
using Lattice::Benchmarks::Trace;

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

/// Particle table as a simulation keeps it.
StdData::SoA particleTable(size_t n) {
    StdData::SoA t;
    t.addCol<PX>(), t.addCol<PY>(), t.addCol<PZ>();
    t.addCol<VX>(), t.addCol<VY>(), t.addCol<VZ>();
    t.addCol<FX>(), t.addCol<FY>(), t.addCol<FZ>();
    t.resize(n);
    return t;
}

StdData::SoA copyOf(const StdData::SoA& from) {
    StdData::SoA to = particleTable(from.size());
    std::ranges::copy(from.bytes(), const_cast<std::byte*>(to.bytes().data()));
    return to;
}

/// Particle table, plus its initial state for prepare().
struct Table {
    explicit Table(size_t n) : table(particleTable(n)), initial(particleTable(n)) {}
    explicit Table(const StdData::SoA& from) : table(copyOf(from)), initial(copyOf(from)) {}
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

/// The particles of the Walls works before a step: inside and around the tube, most in
/// the region, some in the walls. Also the result of a step, to check an implementation.
struct TubeInput {
    StdData::SoA table;
};

TubeInput tubeInput(size_t n) {
    TubeInput input{particleTable(n)};
    std::mt19937 random(2);
    std::uniform_real_distribution<float> radial(0, 4.4f), angle(0, 6.2831853f), height(0, 10), speed(-1, 1);
    auto x = input.table.span<PX>(), y = input.table.span<PY>(), z = input.table.span<PZ>();
    auto vx = input.table.span<VX>(), vy = input.table.span<VY>(), vz = input.table.span<VZ>();
    for (size_t i = 0; i < n; ++i) {
        const float r = radial(random), a = angle(random);
        x[i] = 5 + r * std::cos(a), y[i] = height(random), z[i] = 5 + r * std::sin(a);
        vx[i] = speed(random), vy[i] = speed(random), vz[i] = speed(random);
    }
    return input;
}

/// Every column within 1e-3 of its largest value: the GPU rounds differently.
bool near(const TubeInput& expected, const TubeInput& actual) {
    const auto& a = expected.table;
    const auto& b = actual.table;
    if (a.size() != b.size())
        return false;
    for (const auto& column : {Layout.x, Layout.y, Layout.z, Layout.vx, Layout.vy, Layout.vz, Layout.fx, Layout.fy, Layout.fz}) {
        const auto x = a.span<float>(column), y = b.span<float>(column);
        float largest = 1e-6f, worst = 0;
        for (size_t i = 0; i < x.size(); ++i)
            largest = std::max(largest, std::abs(x[i])), worst = std::max(worst, std::abs(x[i] - y[i]));
        if (worst / largest > 1e-3f)
            return false;
    }
    return true;
}

struct TubeParticles : Lattice::Fixture, Table {
    using Input = TubeInput;
    explicit TubeParticles(const TubeInput& input) : Table(input.table) {}
    void prepare() override { reset(); }
    TubeInput state() const { return {copyOf(table)}; }
};

/// The same particles out of every cache before each iteration: the CPU side of an
/// offloaded step, whose data comes from memory.
struct ColdTubeParticles : TubeParticles {
    using TubeParticles::TubeParticles;
    void prepare() override {
        TubeParticles::prepare();
        Lattice::Benchmarks::evictCaches();
    }
};

::Wgpu::Device& device() {
    static ::Wgpu::Instance instance;
    static ::Wgpu::Device device{instance};
    return device;
}

/// The tube particles resident on the GPU. The initial state is uploaded once into a
/// second table on the device; prepare() resets the working table by a copy on the GPU —
/// resident data never goes through the host between iterations.
struct GpuTubeParticles : TubeParticles {
    // One per point N: the walls' bind groups stay cached as in a simulation, instead of
    // being created anew for the new buffers of every sample.
    static constexpr bool Reuse = true;
    explicit GpuTubeParticles(const TubeInput& input)
        : TubeParticles(input), gpu(device(), "bench/particles"), initialGpu(device(), "bench/initial") {
        gpu.upload(table);
        initialGpu.upload(table);
        // upload() only stages the data: the GPU copies it at the next submit.
        device().flush();
    }
    void prepare() override {
        TubeParticles::prepare();
        ::Wgpu::CommandList commands{device()};
        initialGpu.read([&](const ::Wgpu::GpuSoA::View& from) {
            gpu.read([&](const ::Wgpu::GpuSoA::View& to) {
                commands.copyBuffer(from.buffer, 0, to.buffer, 0, table.bytes().size());
            });
        });
        commands.submit();
        device().wait();
    }
    /// The table on the device, for the check against cpu.
    TubeInput gpuState() {
        TubeInput result{copyOf(table)};
        gpu.read([&](const ::Wgpu::GpuSoA::View& view) {
            device().readBuffer(view.buffer, 0,
                std::span(const_cast<std::byte*>(result.table.bytes().data()), result.table.bytes().size()));
        });
        return result;
    }
    ::Wgpu::GpuSoA gpu, initialGpu;
};

/// One GPU step: record, submit, wait; the GPU time of the pass comes from the timer.
template<class Record>
void stepOnGpu(Trace& trace, ::Wgpu::Timer& timer, Record&& record) {
    ::Wgpu::CommandList commands{device()};
    {
        auto prepare = trace.span(Phase::Prepare);
        record(commands);
    }
    {
        auto issue = trace.span(Phase::Issue);
        commands.submit();
    }
    {
        auto complete = trace.span(Phase::Complete);
        device().wait();
    }
    trace.deferIntervals(timer);
}

/// A CPU stage over the rows on `workers` threads (Tasks::Workers) of the pool.
template<class Stage>
void onCpu(Run& run, StdData::SoA& table, Stage&& stage) {
    auto execute = run.trace().span(Phase::Execute);
    Tasks::benchPool().forPartitions(table.size(), run.value<Tasks::Workers>(),
        [&](size_t begin, size_t end) { stage(table, begin, end); });
}

/// Back into the particle table after a step on the GPU, as an offloading simulation would.
void readBack(Trace& trace, GpuTubeParticles& p) {
    const size_t bytes = p.table.bytes().size();
    auto readback = trace.span(Phase::TransferOut);
    p.gpu.read([&](const ::Wgpu::GpuSoA::View& view) {
        device().readBuffer(view.buffer, 0, std::span(const_cast<std::byte*>(p.table.bytes().data()), bytes));
    });
    readback.bytes(bytes);
}

}

BENCH_GROUP(Walls) { bench.config.sizes = Sizes; }

BENCH_WORK(Walls, Constrain, TubeInput, "hard tube walls: project and reflect the ~10% in walls") {
    work.input(tubeInput);
    work.check(near);
    work.reference("cpu");
}

BENCH_IMPLEMENTATION(Walls, Constrain, "cpu", TubeParticles) {
    const Geometry geometry{tube()};
    const Walls walls{geometry, Layout};
    impl.axis<Tasks::Workers>();
    impl.output([](TubeParticles& p) { return p.state(); });
    impl.measure([&](TubeParticles& p, Run& run) {
        onCpu(run, p.table, [&](StdData::SoA& t, size_t begin, size_t end) { walls.constrain(t, begin, end); });
    });
}

BENCH_IMPLEMENTATION(Walls, Constrain, "gpu", GpuTubeParticles) {
    const Boundaries::Wgpu::GpuGeometry gpuGeometry{device(), tube().compile()};
    Boundaries::Wgpu::GpuWalls gpuWalls{gpuGeometry, Layout};
    ::Wgpu::Timer timer{device(), 1024, "bench/walls"};
    impl.output([](GpuTubeParticles& p) { return p.gpuState(); });
    impl.measure([&](GpuTubeParticles& p, Run& run) {
        stepOnGpu(run.trace(), timer, [&](::Wgpu::CommandList& commands) {
            gpuWalls.constrain(commands, p.gpu, p.table.size(), &timer);
        });
    });
}

BENCH_WORK(Walls, ConstrainOffload, TubeInput,
           "hard tube walls, data from memory: CPU from a cold cache, GPU with the copies there and back") {
    work.input(tubeInput);
    work.check(near);
    work.reference("cpu");
}

BENCH_IMPLEMENTATION(Walls, ConstrainOffload, "cpu", ColdTubeParticles) {
    const Geometry geometry{tube()};
    const Walls walls{geometry, Layout};
    impl.axis<Tasks::Workers>();
    impl.output([](ColdTubeParticles& p) { return p.state(); });
    impl.measure([&](ColdTubeParticles& p, Run& run) {
        onCpu(run, p.table, [&](StdData::SoA& t, size_t begin, size_t end) { walls.constrain(t, begin, end); });
    });
}

BENCH_IMPLEMENTATION(Walls, ConstrainOffload, "gpu", GpuTubeParticles) {
    const Boundaries::Wgpu::GpuGeometry gpuGeometry{device(), tube().compile()};
    Boundaries::Wgpu::GpuWalls gpuWalls{gpuGeometry, Layout};
    ::Wgpu::Timer timer{device(), 1024, "bench/walls"};
    // The result is back in the host table, where the step leaves it.
    impl.output([](GpuTubeParticles& p) { return p.state(); });
    impl.measure([&](GpuTubeParticles& p, Run& run) {
        Trace& trace = run.trace();
        {
            // upload() only stages; flush() makes the copy to the device part of this phase.
            auto upload = trace.span(Phase::TransferIn);
            p.gpu.upload(p.table);
            device().flush();
            upload.bytes(p.table.bytes().size());
        }
        stepOnGpu(trace, timer, [&](::Wgpu::CommandList& commands) {
            gpuWalls.constrain(commands, p.gpu, p.table.size(), &timer);
        });
        readBack(trace, p);
    });
}

BENCH_WORK(Walls, SoftForces, TubeInput, "soft tube walls, layer 0.5 thick") {
    work.input(tubeInput);
    work.check(near);
    work.reference("cpu");
}

BENCH_IMPLEMENTATION(Walls, SoftForces, "cpu", TubeParticles) {
    const Geometry geometry{tube()};
    const Walls walls{geometry, Layout, {Laws::Soft::make(10, 0.5f, 6)}};
    impl.axis<Tasks::Workers>();
    impl.output([](TubeParticles& p) { return p.state(); });
    impl.measure([&](TubeParticles& p, Run& run) {
        onCpu(run, p.table, [&](StdData::SoA& t, size_t begin, size_t end) { walls.addForces(t, begin, end); });
    });
}

BENCH_IMPLEMENTATION(Walls, SoftForces, "gpu", GpuTubeParticles) {
    const Boundaries::Wgpu::GpuGeometry gpuGeometry{device(), tube().compile()};
    Boundaries::Wgpu::GpuWalls gpuWalls{gpuGeometry, Layout, {Laws::Soft::make(10, 0.5f, 6)}};
    ::Wgpu::Timer timer{device(), 1024, "bench/walls"};
    impl.output([](GpuTubeParticles& p) { return p.gpuState(); });
    impl.measure([&](GpuTubeParticles& p, Run& run) {
        stepOnGpu(run.trace(), timer, [&](::Wgpu::CommandList& commands) {
            gpuWalls.addForces(commands, p.gpu, p.table.size(), &timer);
        });
    });
}

// [filter] [--filter <group|bench|mask>] [--tsv <file>] [--sweep <axis>] [--axis <axis>=<v1,v2>]
// [--list]; a group run also compares the implementations of each work.
int main(int argc, char** argv) {
    return Lattice::Benchmarks::main(argc, argv);
}
