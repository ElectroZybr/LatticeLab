#include "SdfMesh.hpp"

#include <algorithm>
#include <cmath>
#include <span>
#include <string>

#include <BoundariesWgpu/Codegen.hpp>
#include <BoundariesWgpu/GeometrySource.hpp>
#include <Lattice/Tools/Exception.hpp>
#include <Wgpu/CommandList.hpp>
#include <Wgpu/ComputePass.hpp>
#include <Wgpu/ShaderSource.hpp>

namespace {

using Error = Lattice::Exception<SdfMesh>;

constexpr unsigned char Shader[] = {
#include "SdfMesh.wgsl.h"
};
static_assert(Wgpu::shaderConstant(Shader, "BRICK") == SdfMesh::Brick);

struct Params {
    glm::vec4 origin;
    glm::uvec4 bricks;
    glm::uvec4 capacity;
};

struct Counters {
    uint32_t slots, vertices, indices, pad;
};

constexpr uint32_t BrickCells = SdfMesh::Brick * SdfMesh::Brick * SdfMesh::Brick;
constexpr uint32_t BrickCorners = (SdfMesh::Brick + 1) * (SdfMesh::Brick + 1) * (SdfMesh::Brick + 1);

constexpr std::array<Wgpu::LayoutEntry, 9> Entries{{
    {.binding = 0, .type = Wgpu::BindingType::ReadOnlyStorage, .visibility = Wgpu::ShaderStage::Compute},
    {.binding = 1, .type = Wgpu::BindingType::Uniform, .visibility = Wgpu::ShaderStage::Compute},
    {.binding = 2, .type = Wgpu::BindingType::Storage, .visibility = Wgpu::ShaderStage::Compute},
    {.binding = 3, .type = Wgpu::BindingType::Storage, .visibility = Wgpu::ShaderStage::Compute},
    {.binding = 4, .type = Wgpu::BindingType::Storage, .visibility = Wgpu::ShaderStage::Compute},
    {.binding = 5, .type = Wgpu::BindingType::Storage, .visibility = Wgpu::ShaderStage::Compute},
    {.binding = 6, .type = Wgpu::BindingType::Storage, .visibility = Wgpu::ShaderStage::Compute},
    {.binding = 7, .type = Wgpu::BindingType::Storage, .visibility = Wgpu::ShaderStage::Compute},
    {.binding = 8, .type = Wgpu::BindingType::Storage, .visibility = Wgpu::ShaderStage::Compute},
}};

std::unique_ptr<Wgpu::Buffer> storage(Wgpu::Device& device, uint64_t bytes, const char* label) {
    return std::make_unique<Wgpu::Buffer>(device, Wgpu::BufferDesc{.size = std::max<uint64_t>(bytes, 4),
        .usage = Wgpu::BufferUsage::Storage, .label = label});
}

}

struct SdfMesh::Pipelines {
    Pipelines(Wgpu::Device& device, Wgpu::BindingLayout& layout, const Boundaries::Program& program)
        : shader(device, {.source = std::string(Boundaries::Wgpu::geometrySource()) +
              Boundaries::Wgpu::generateDistance(program, "sdfDistance") + std::string(Wgpu::shaderText(Shader)),
              .label = "sdf-mesh"}),
          clear(device, shader, {.layouts = {&layout}, .entry = "clear", .label = "sdf-mesh/clear"}),
          classify(device, shader, {.layouts = {&layout}, .entry = "classify", .label = "sdf-mesh/classify"}),
          sample(device, shader, {.layouts = {&layout}, .entry = "sample", .label = "sdf-mesh/sample"}),
          place(device, shader, {.layouts = {&layout}, .entry = "place", .label = "sdf-mesh/place"}),
          connect(device, shader, {.layouts = {&layout}, .entry = "connect", .label = "sdf-mesh/connect"}) {}

    Wgpu::Shader shader;
    Wgpu::ComputePipeline clear, classify, sample, place, connect;
};

SdfMesh::SdfMesh(Wgpu::Device& device, const Boundaries::Wgpu::GpuGeometry& geometry,
                 const Boundaries::Space& space, uint32_t resolution)
    : GpuMesh(device, "sdf-mesh"),
      resolution(resolution),
      geometry_(geometry),
      space_(space),
      device_(device),
      layout_(device, Entries, "sdf-mesh"),
      params_(device, {.size = sizeof(Params), .usage = Wgpu::BufferUsage::Uniform | Wgpu::BufferUsage::CopyDestination,
          .label = "sdf-mesh/params"}),
      counters_(device, {.size = sizeof(Counters),
          .usage = Wgpu::BufferUsage::Storage | Wgpu::BufferUsage::CopyDestination | Wgpu::BufferUsage::CopySource,
          .label = "sdf-mesh/counters"}) {
    build();
}

SdfMesh::~SdfMesh() = default;

bool SdfMesh::update() {
    if (geometry_.version() == builtVersion_ && resolution == builtResolution_)
        return false;
    build();
    return true;
}

void SdfMesh::prepare() {
    if (!resolution)
        throw Error("resolution must be positive");
    const glm::vec3 size = space_.size();
    const float cell = std::max({size.x, size.y, size.z}) / float(resolution);
    const glm::uvec3 bricks = glm::max(glm::uvec3(glm::ceil(size / (cell * float(Brick)))), glm::uvec3(1u));
    grid_ = {.origin = space_.origin(), .cellSize = glm::vec3(cell), .cells = bricks * Brick};
    if (bricks != bricks_) {
        bricks_ = bricks;
        const uint64_t total = uint64_t(bricks.x) * bricks.y * bricks.z;
        slotOf_ = storage(device_, total * 4, "sdf-mesh/slot-of");
        // A first guess from the area of the box in bricks; run() grows what overflows.
        const uint64_t area = 2ull * (uint64_t(bricks.x) * bricks.y + uint64_t(bricks.y) * bricks.z +
            uint64_t(bricks.z) * bricks.x);
        capacity_ = {uint32_t(std::min(total, area * 2)), 0, 0};
        capacity_[1] = capacity_[0] * 128;
        capacity_[2] = capacity_[1] * 7;
        brickOf_.reset();
    }
    const auto structure = Boundaries::Wgpu::structure(geometry_.program());
    if (!pipelines_ || structure != structure_) {
        structure_ = structure;
        pipelines_.reset();
        pipelines_ = std::make_unique<Pipelines>(device_, layout_, geometry_.program());
    }
}

bool SdfMesh::run() {
    if (!brickOf_ || brickOf_->size() < uint64_t(capacity_[0]) * 4) {
        brickOf_ = storage(device_, uint64_t(capacity_[0]) * 4, "sdf-mesh/brick-of");
        samples_ = storage(device_, uint64_t(capacity_[0]) * BrickCorners * 4, "sdf-mesh/samples");
        cellVertex_ = storage(device_, uint64_t(capacity_[0]) * BrickCells * 4, "sdf-mesh/cell-vertex");
    }
    reserve(uint64_t(capacity_[1]) * sizeof(MeshVertex), uint64_t(capacity_[2]) * sizeof(uint32_t));

    const std::array bindings{
        Wgpu::Binding{.binding = 0, .buffer = &geometry_.code()},
        Wgpu::Binding{.binding = 1, .buffer = &params_},
        Wgpu::Binding{.binding = 2, .buffer = &counters_},
        Wgpu::Binding{.binding = 3, .buffer = slotOf_.get()},
        Wgpu::Binding{.binding = 4, .buffer = brickOf_.get()},
        Wgpu::Binding{.binding = 5, .buffer = samples_.get()},
        Wgpu::Binding{.binding = 6, .buffer = cellVertex_.get()},
        Wgpu::Binding{.binding = 7, .buffer = vertices()},
        Wgpu::Binding{.binding = 8, .buffer = indices()},
    };
    set_.reset();
    set_ = std::make_unique<Wgpu::BindingSet>(device_, layout_, bindings);

    const uint64_t total = uint64_t(bricks_.x) * bricks_.y * bricks_.z;
    const Params params{
        .origin = glm::vec4(grid_.origin, grid_.cellSize.x),
        .bricks = glm::uvec4(bricks_, uint32_t(total)),
        .capacity = glm::uvec4(capacity_[0], capacity_[1], capacity_[2], 0u),
    };
    device_.writeBuffer(params_, 0, std::as_bytes(std::span(&params, 1)));
    const Counters zero{};
    device_.writeBuffer(counters_, 0, std::as_bytes(std::span(&zero, 1)));

    Wgpu::CommandList commands{device_};
    {
        Wgpu::ComputePass pass{commands, "sdf-mesh"};
        pass.setBindings(0, *set_);
        pass.setPipeline(pipelines_->clear);
        pass.dispatchCovering(total, 64);
        pass.setPipeline(pipelines_->classify);
        pass.dispatchCovering(total, 64);
        for (auto* stage : {&pipelines_->sample, &pipelines_->place, &pipelines_->connect}) {
            pass.setPipeline(*stage);
            pass.dispatchCovering(uint64_t(capacity_[0]) * 64, 64);
        }
    }
    commands.submit();

    Counters counts{};
    device_.readBuffer(counters_, 0, std::as_writable_bytes(std::span(&counts, 1)));
    const std::array<uint32_t, 3> needed{counts.slots, counts.vertices, counts.indices};
    bool fits = true;
    for (size_t i = 0; i < 3; ++i)
        if (needed[i] > capacity_[i]) {
            capacity_[i] = needed[i] + needed[i] / 4;
            fits = false;
        }
    activeBricks_ = counts.slots;
    vertexCount_ = counts.vertices;
    setIndexCount(fits ? counts.indices : 0);
    return fits;
}

void SdfMesh::build() {
    const uint64_t before = Wgpu::errorCount();
    prepare();
    // An overflow grows the capacities and runs again; the counts of the first run are exact.
    if (!run() && !run())
        throw Error("surface does not fit the grown buffers");
    if (Wgpu::errorCount() != before)
        throw Error("the GPU rejected the surface nets passes (see the WGPU error above)");
    builtVersion_ = geometry_.version();
    builtResolution_ = resolution;
}
