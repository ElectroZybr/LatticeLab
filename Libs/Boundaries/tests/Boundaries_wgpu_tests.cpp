// The GPU backend against the CPU one on the same data; skipped without an adapter.
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <BoundariesCpu/Geometry.hpp>
#include <BoundariesCpu/Walls.hpp>
#include <BoundariesWgpu/GpuGeometry.hpp>
#include <BoundariesWgpu/GpuWalls.hpp>
#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Tools/TypeName.hpp>
#include <Wgpu/CommandList.hpp>
#include <Wgpu/Device.hpp>
#include <Wgpu/GpuSoA.hpp>

using namespace Boundaries;
namespace Gpu = Boundaries::Wgpu;

namespace {

void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

struct PX { using type = float; };
struct PY { using type = float; };
struct PZ { using type = float; };
struct VX { using type = float; };
struct VY { using type = float; };
struct VZ { using type = float; };
struct FX { using type = float; };
struct FY { using type = float; };
struct FZ { using type = float; };

const ParticleLayout layout{
    std::string(Lattice::typeName<PX>()), std::string(Lattice::typeName<PY>()), std::string(Lattice::typeName<PZ>()),
    std::string(Lattice::typeName<VX>()), std::string(Lattice::typeName<VY>()), std::string(Lattice::typeName<VZ>()),
    std::string(Lattice::typeName<FX>()), std::string(Lattice::typeName<FY>()), std::string(Lattice::typeName<FZ>())};

StdData::SoA makeTable(size_t n) {
    StdData::SoA table;
    table.addCol<PX>(), table.addCol<PY>(), table.addCol<PZ>();
    table.addCol<VX>(), table.addCol<VY>(), table.addCol<VZ>();
    table.addCol<FX>(), table.addCol<FY>(), table.addCol<FZ>();
    table.resize(n);
    return table;
}

/// Random particles in [-span, span]^3 with random velocities.
StdData::SoA randomParticles(size_t n, float span, unsigned seed) {
    auto table = makeTable(n);
    std::mt19937 random(seed);
    std::uniform_real_distribution<float> coordinate(-span, span), speed(-1, 1);
    for (auto* name : {&layout.x, &layout.y, &layout.z})
        for (float& value : table.span<float>(*name)) value = coordinate(random);
    for (auto* name : {&layout.vx, &layout.vy, &layout.vz})
        for (float& value : table.span<float>(*name)) value = speed(random);
    return table;
}

/// GPU block read back into a table of the same layout.
StdData::SoA readBack(::Wgpu::Device& device, ::Wgpu::GpuSoA& gpu, const StdData::SoA& like) {
    auto result = makeTable(like.size());
    gpu.read([&](const ::Wgpu::GpuSoA::View& view) {
        const auto bytes = device.readBuffer(view.buffer, 0, like.bytes().size());
        std::memcpy(const_cast<std::byte*>(result.bytes().data()), bytes.data(), bytes.size());
    });
    return result;
}

float maxDifference(const StdData::SoA& a, const StdData::SoA& b, const std::string& column) {
    const auto x = a.span<float>(column), y = b.span<float>(column);
    float worst = 0;
    for (size_t i = 0; i < x.size(); ++i)
        worst = std::max(worst, std::abs(x[i] - y[i]));
    return worst;
}

const auto quarter = glm::angleAxis(glm::radians(90.0f), glm::normalize(glm::vec3(1, 1, 0)));

/// Every primitive and operation alone, then combined.
std::vector<std::pair<const char*, Shape>> shapes() {
    std::vector<std::pair<const char*, Shape>> list;
    list.emplace_back("sphere", Shape::sphere(1.2f));
    list.emplace_back("box", Shape::box({1, 0.7f, 1.4f}));
    list.emplace_back("capsule", Shape::capsule({-1, 0, 0}, {1, 0.5f, 0}, 0.4f));
    list.emplace_back("cylinder", Shape::cylinder(0.8f, 1.1f));
    list.emplace_back("half-space", Shape::halfSpace({0.3f, 1, 0.2f}, 0.5f));
    list.emplace_back("infinite cylinder", Shape::infiniteCylinder(0.9f));
    list.emplace_back("union", Shape::sphere(1) | std::move(Shape::box({0.5f, 0.5f, 0.5f})).translated({1, 0, 0}));
    list.emplace_back("intersection", Shape::sphere(1.3f) & Shape::box({1, 1, 1}));
    list.emplace_back("subtraction", Shape::box({1.5f, 1.5f, 1.5f}) - Shape::sphere(1));
    list.emplace_back("smooth union", Shape::smoothUnion(Shape::sphere(0.8f),
        std::move(Shape::sphere(0.8f)).translated({1.2f, 0, 0}), 0.5f));
    list.emplace_back("round", std::move(Shape::box({0.8f, 0.8f, 0.8f})).rounded(0.2f));
    list.emplace_back("shell", std::move(Shape::sphere(1.2f)).shell(0.15f));
    list.emplace_back("transforms", std::move(std::move(Shape::box({1, 0.3f, 0.5f})).rotated(quarter).scaled(1.5f))
        .translated({0.2f, -0.3f, 0.1f}));
    list.emplace_back("tube with mixer", Shape::capsule({0, -2, 0}, {0, 2, 0}, 1.8f) -
        std::move(Shape::smoothUnion(Shape::capsule({0, -1, 0}, {0, 2.5f, 0}, 0.2f),
            std::move(Shape::box({1, 0.2f, 0.15f})).translated({0, -1, 0}), 0.2f)).rounded(0.05f));
    return list;
}

void distancesMatchTheCpu(::Wgpu::Device& device) {
    constexpr size_t n = 4096;
    const auto points = randomParticles(n, 2.5f, 3);
    ::Wgpu::GpuSoA gpu{device, "points"};
    gpu.upload(points);
    ::Wgpu::Buffer out{device, {.size = n * sizeof(float), .usage = ::Wgpu::BufferUsage::Storage |
        ::Wgpu::BufferUsage::CopySource, .label = "distances"}};
    for (auto& [name, shape] : shapes()) {
        const Cpu::Geometry cpu{shape};
        const Gpu::GpuGeometry geometry{device, shape.compile()};
        Gpu::GpuDistances distances{geometry};
        ::Wgpu::CommandList commands{device};
        distances.evaluate(commands, gpu, layout, n, out);
        commands.submit();
        const auto bytes = device.readBuffer(out, 0, n * sizeof(float));
        const auto* got = reinterpret_cast<const float*>(bytes.data());
        std::vector<float> expected(n);
        cpu.evaluate(points.span<float>(layout.x), points.span<float>(layout.y), points.span<float>(layout.z), expected);
        float worst = 0;
        for (size_t i = 0; i < n; ++i)
            worst = std::max(worst, std::abs(got[i] - expected[i]));
        check(worst < 1e-5f, std::string("distances of ") + name + ": max difference " + std::to_string(worst));
    }
}

void wallsMatchTheCpu(::Wgpu::Device& device) {
    const Shape vessel = (std::move(Shape::capsule({0, -2, 0}, {0, 2, 0}, 1.8f)).material(0) -
        std::move(Shape::sphere(0.6f)).translated({0.8f, 0, 0}).material(1)) -
        std::move(Shape::box({0.3f, 2.5f, 0.3f})).material(2);
    const std::vector<Material> materials{Laws::Hard::make(0.8f), Laws::Soft::make(5, 0.4f, 6), Laws::Hard::make()};
    constexpr size_t n = 20'000;   // many particles in walls: everything outside the tube is in a wall

    const Cpu::Geometry cpuGeometry{vessel};
    const Cpu::Walls cpu{cpuGeometry, layout, materials};
    const Gpu::GpuGeometry gpuGeometry{device, vessel.compile()};
    Gpu::GpuWalls gpu{gpuGeometry, layout, materials};

    auto onCpu = randomParticles(n, 2.4f, 5);
    ::Wgpu::GpuSoA table{device, "particles"};
    table.upload(onCpu);

    cpu.addForces(onCpu, n);
    cpu.constrain(onCpu, n);
    ::Wgpu::CommandList commands{device};
    gpu.addForces(commands, table, n);
    gpu.constrain(commands, table, n);
    commands.submit();
    const auto onGpu = readBack(device, table, onCpu);

    for (auto* column : {&layout.x, &layout.y, &layout.z, &layout.vx, &layout.vy, &layout.vz})
        check(maxDifference(onCpu, onGpu, *column) < 1e-4f, "walls: " + *column + " differs by " +
            std::to_string(maxDifference(onCpu, onGpu, *column)));
    // Forces grow fast with depth (exponent 6): compare relative to the largest force.
    for (auto* column : {&layout.fx, &layout.fy, &layout.fz}) {
        float largest = 1e-6f;
        for (float f : onCpu.span<float>(*column)) largest = std::max(largest, std::abs(f));
        check(maxDifference(onCpu, onGpu, *column) / largest < 1e-3f, "walls: " + *column + " differs");
    }
}

void deepProgramsAreRejected(::Wgpu::Device& device) {
    // a | (b | (c | ...)): the code is a b c ... U U U, every sphere stays on the stack.
    std::vector<Shape> leaves;
    for (int i = 0; i < 17; ++i)
        leaves.push_back(Shape::sphere(1));
    Shape tree = std::move(leaves.back());
    leaves.pop_back();
    while (!leaves.empty()) {
        tree = std::move(leaves.back()) | std::move(tree);
        leaves.pop_back();
    }
    bool thrown = false;
    try {
        Gpu::GpuGeometry geometry{device, tree.compile()};
    } catch (const Lattice::ExceptionBase&) {
        thrown = true;
    }
    check(tree.compile().depth > Gpu::MaxDepth && thrown, "a program deeper than the GPU stack is rejected");
}

}

/// Called from main (Boundaries_tests.cpp): one test binary for the model and every backend.
void wgpuMatchesTheCpu() {
    ::Wgpu::Instance instance;
    std::unique_ptr<::Wgpu::Device> device;
    try {
        device = std::make_unique<::Wgpu::Device>(instance);
    } catch (const std::exception& error) {
        std::cout << "Boundaries.wgpu: no adapter, skipped (" << error.what() << ")\n";
        return;
    }
    const auto errors = ::Wgpu::errorCount();
    distancesMatchTheCpu(*device);
    wallsMatchTheCpu(*device);
    deepProgramsAreRejected(*device);
    check(::Wgpu::errorCount() == errors, "no GPU validation errors");
    check(device->resources().size() == 0, "every GPU resource is released");
    std::cout << "Boundaries.wgpu: distances of every primitive and operation, walls and depth limit match the CPU\n";
}
