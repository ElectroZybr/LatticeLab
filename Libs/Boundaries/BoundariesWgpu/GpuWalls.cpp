#include "GpuWalls.hpp"

#include <array>
#include <span>
#include <string>

#include <Boundaries/Materials.hpp>
#include <Lattice/Tools/Exception.hpp>
#include <Wgpu/ComputePass.hpp>
#include <Wgpu/ShaderSource.hpp>

#include "GpuGeometry.hpp"

namespace Boundaries::Wgpu {

using Error = Lattice::Exception<GpuWalls>;

namespace {

// Shaders embedded at build time (utils.bin2c): the geometry machine, then a kernel.
constexpr unsigned char Geometry[] = {
#include "Geometry.wgsl.h"
};

constexpr unsigned char Walls[] = {
#include "Walls.wgsl.h"
};

constexpr unsigned char Distances[] = {
#include "Distances.wgsl.h"
};

// The shaders mirror the C++ code: a mismatch fails the build, not a simulation.
constexpr bool sameOp(std::string_view name, Op op) { return ::Wgpu::shaderConstant(Geometry, name) == int64_t(op); }
static_assert(::Wgpu::shaderConstant(Geometry, "GEOMETRY_DEPTH") == MaxDepth);
static_assert(sameOp("OP_SPHERE", Op::Sphere) && sameOp("OP_BOX", Op::Box) && sameOp("OP_CAPSULE", Op::Capsule) &&
              sameOp("OP_CYLINDER", Op::Cylinder) && sameOp("OP_HALF_SPACE", Op::HalfSpace) &&
              sameOp("OP_INFINITE_CYLINDER", Op::InfiniteCylinder) && sameOp("OP_UNION", Op::Union) &&
              sameOp("OP_INTERSECT", Op::Intersect) && sameOp("OP_SUBTRACT", Op::Subtract) &&
              sameOp("OP_SMOOTH_UNION", Op::SmoothUnion) && sameOp("OP_ROUND", Op::Round) && sameOp("OP_SHELL", Op::Shell));
// The machine relies on the order primitives < binary < unary.
static_assert(Op::InfiniteCylinder < Op::Union && Op::SmoothUnion < Op::Round && Op::Round < Op::Shell);
static_assert(::Wgpu::shaderConstant(Walls, "WORKGROUP") == GpuWalls::WorkgroupSize);
static_assert(::Wgpu::shaderConstant(Walls, "LAW_HARD") == Laws::Hard::id);
static_assert(::Wgpu::shaderConstant(Walls, "LAW_SOFT") == Laws::Soft::id);

struct Params {
    uint32_t rows, x, y, z;
    uint32_t a, b, c, passes;
    float forceRange, pad0, pad1, pad2;
};

/// Word offset of a f32 column of the table.
uint32_t column(const ::Wgpu::GpuSoA::View& view, const std::string& name) {
    std::string names;
    for (const auto& column : view.columns) {
        if (column.name == name) {
            if (!column.type.is<float>() || column.offset % 4)
                throw Error("column '{}' is not a word-aligned f32 column", name);
            return uint32_t(column.offset / 4);
        }
        names += (names.empty() ? "" : ", ") + column.name;
    }
    throw Error("particle table has no column '{}'; columns: [{}]", name, names);
}

std::string withGeometry(const unsigned char* kernel) {
    return std::string(::Wgpu::shaderText(Geometry)) + reinterpret_cast<const char*>(kernel);
}

constexpr std::array<::Wgpu::LayoutEntry, 4> WallEntries{{
    {.binding = 0, .type = ::Wgpu::BindingType::ReadOnlyStorage, .visibility = ::Wgpu::ShaderStage::Compute},
    {.binding = 1, .type = ::Wgpu::BindingType::ReadOnlyStorage, .visibility = ::Wgpu::ShaderStage::Compute},
    {.binding = 2, .type = ::Wgpu::BindingType::Storage, .visibility = ::Wgpu::ShaderStage::Compute},
    {.binding = 3, .type = ::Wgpu::BindingType::Uniform, .visibility = ::Wgpu::ShaderStage::Compute},
}};

constexpr std::array<::Wgpu::LayoutEntry, 4> DistanceEntries{{
    {.binding = 0, .type = ::Wgpu::BindingType::ReadOnlyStorage, .visibility = ::Wgpu::ShaderStage::Compute},
    {.binding = 1, .type = ::Wgpu::BindingType::ReadOnlyStorage, .visibility = ::Wgpu::ShaderStage::Compute},
    {.binding = 2, .type = ::Wgpu::BindingType::Storage, .visibility = ::Wgpu::ShaderStage::Compute},
    {.binding = 3, .type = ::Wgpu::BindingType::Uniform, .visibility = ::Wgpu::ShaderStage::Compute},
}};

::Wgpu::BufferDesc uniform(size_t size, const char* label) {
    return {.size = size, .usage = ::Wgpu::BufferUsage::Uniform | ::Wgpu::BufferUsage::CopyDestination, .label = label};
}

}

/// One stage: its pipeline, parameters and the bind group over the current buffers.
struct GpuWalls::Stage {
    Stage(::Wgpu::Device& device, ::Wgpu::Shader& shader, ::Wgpu::BindingLayout& layout, const char* entry, bool velocities)
        : pipeline(device, shader, {.layouts = {&layout}, .entry = entry, .label = std::string("walls/") + entry}),
          params(device, uniform(sizeof(Params), "walls/params")),
          velocities(velocities), name(entry) {}

    ::Wgpu::ComputePipeline pipeline;
    ::Wgpu::Buffer params;
    bool velocities;
    std::string_view name;   // pass label and timer interval
    std::unique_ptr<::Wgpu::BindingSet> set;
    const ::Wgpu::Buffer* table = nullptr;
    uint64_t tableGeneration = ~0ull, geometryGeneration = ~0ull;
};

namespace {
/// wgpu reports invalid shaders and pipelines when they are created: surface it there
/// instead of as a failed submit later.
void requireNoNewErrors(uint64_t before, const char* what) {
    if (::Wgpu::errorCount() != before)
        throw Error("{}: the GPU rejected the shader or pipeline (see the WGPU error above)", what);
}
}

GpuWalls::GpuWalls(const GpuGeometry& geometry, ParticleLayout layout, std::vector<Material> materials)
    : errorsBefore_(::Wgpu::errorCount()),
      geometry_(geometry),
      layout_(std::move(layout)),
      materials_(std::move(materials)),
      forceRange_(checkMaterials(geometry_.program(), materials_)),
      materialBuffer_(geometry.device(), {.size = materials_.size() * sizeof(Material),
          .usage = ::Wgpu::BufferUsage::Storage | ::Wgpu::BufferUsage::CopyDestination, .label = "walls/materials"}),
      bindings_(geometry.device(), WallEntries, "walls"),
      shader_(geometry.device(), {.source = withGeometry(Walls), .label = "walls"}),
      constrain_(std::make_unique<Stage>(geometry.device(), shader_, bindings_, "constrain", true)),
      forces_(std::make_unique<Stage>(geometry.device(), shader_, bindings_, "forces", false)) {
    geometry.device().writeBuffer(materialBuffer_, 0, std::as_bytes(std::span(materials_)));
    requireNoNewErrors(errorsBefore_, "walls");
}

GpuWalls::~GpuWalls() = default;

void GpuWalls::record(::Wgpu::CommandList& commands, ::Wgpu::GpuSoA& particles, size_t rows, Stage& stage,
                      ::Wgpu::Timer* timer) {
    if (!rows)
        return;
    auto& device = geometry_.device();
    particles.read([&](const ::Wgpu::GpuSoA::View& view) {
        if (rows > view.rows)
            throw Error("{} rows of a table with {}", rows, view.rows);
        const bool v = stage.velocities;
        const Params params{
            .rows = uint32_t(rows),
            .x = column(view, layout_.x), .y = column(view, layout_.y), .z = column(view, layout_.z),
            .a = column(view, v ? layout_.vx : layout_.fx), .b = column(view, v ? layout_.vy : layout_.fy),
            .c = column(view, v ? layout_.vz : layout_.fz),
            .passes = uint32_t(Laws::Hard::ProjectionPasses),
            .forceRange = forceRange_, .pad0 = 0, .pad1 = 0, .pad2 = 0,
        };
        device.writeBuffer(stage.params, 0, std::as_bytes(std::span(&params, 1)));
        if (!stage.set || stage.table != &view.buffer || stage.tableGeneration != view.generation ||
            stage.geometryGeneration != geometry_.generation()) {
            const std::array bindings{
                ::Wgpu::Binding{.binding = 0, .buffer = &geometry_.code()},
                ::Wgpu::Binding{.binding = 1, .buffer = &materialBuffer_},
                ::Wgpu::Binding{.binding = 2, .buffer = &view.buffer},
                ::Wgpu::Binding{.binding = 3, .buffer = &stage.params},
            };
            stage.set.reset();
            stage.set = std::make_unique<::Wgpu::BindingSet>(device, bindings_, bindings);
            stage.table = &view.buffer;
            stage.tableGeneration = view.generation;
            stage.geometryGeneration = geometry_.generation();
        }
    });
    ::Wgpu::ComputePass pass{commands, stage.name, timer};
    pass.setPipeline(stage.pipeline);
    pass.setBindings(0, *stage.set);
    pass.dispatch(::Wgpu::workgroups(rows, WorkgroupSize));
}

void GpuWalls::addForces(::Wgpu::CommandList& commands, ::Wgpu::GpuSoA& particles, size_t rows, ::Wgpu::Timer* timer) {
    if (forceRange_ > 0.0f)
        record(commands, particles, rows, *forces_, timer);
}

void GpuWalls::constrain(::Wgpu::CommandList& commands, ::Wgpu::GpuSoA& particles, size_t rows, ::Wgpu::Timer* timer) {
    record(commands, particles, rows, *constrain_, timer);
}

struct GpuDistances::State {
    explicit State(::Wgpu::Device& device)
        : layout(device, DistanceEntries, "distances"),
          shader(device, {.source = withGeometry(Distances), .label = "distances"}),
          pipeline(device, shader, {.layouts = {&layout}, .label = "distances"}),
          params(device, uniform(16, "distances/params")) {}

    ::Wgpu::BindingLayout layout;
    ::Wgpu::Shader shader;
    ::Wgpu::ComputePipeline pipeline;
    ::Wgpu::Buffer params;
    std::unique_ptr<::Wgpu::BindingSet> set;   // kept until the next call
};

GpuDistances::GpuDistances(const GpuGeometry& geometry) : geometry_(geometry) {
    const uint64_t before = ::Wgpu::errorCount();
    state_ = std::make_unique<State>(geometry.device());
    requireNoNewErrors(before, "distances");
}

GpuDistances::~GpuDistances() = default;

void GpuDistances::evaluate(::Wgpu::CommandList& commands, ::Wgpu::GpuSoA& points, const ParticleLayout& layout,
                            size_t rows, ::Wgpu::Buffer& distances) {
    if (distances.size() < rows * sizeof(float))
        throw Lattice::Exception<GpuDistances>("output of {} bytes for {} distances", distances.size(), rows);
    auto& set = state_->set;
    points.read([&](const ::Wgpu::GpuSoA::View& view) {
        const std::array<uint32_t, 4> params{uint32_t(rows), column(view, layout.x), column(view, layout.y),
            column(view, layout.z)};
        geometry_.device().writeBuffer(state_->params, 0, std::as_bytes(std::span(params)));
        const std::array bindings{
            ::Wgpu::Binding{.binding = 0, .buffer = &geometry_.code()},
            ::Wgpu::Binding{.binding = 1, .buffer = &view.buffer},
            ::Wgpu::Binding{.binding = 2, .buffer = &distances},
            ::Wgpu::Binding{.binding = 3, .buffer = &state_->params},
        };
        set.reset();
        set = std::make_unique<::Wgpu::BindingSet>(geometry_.device(), state_->layout, bindings);
    });
    ::Wgpu::ComputePass pass{commands, "distances"};
    pass.setPipeline(state_->pipeline);
    pass.setBindings(0, *set);
    pass.dispatch(::Wgpu::workgroups(rows, 64));
}

}
