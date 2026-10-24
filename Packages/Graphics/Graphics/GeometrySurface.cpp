#include "GeometrySurface.hpp"

#include <array>
#include <span>
#include <string>

#include <BoundariesWgpu/GeometrySource.hpp>
#include <Wgpu/ShaderSource.hpp>

namespace {

constexpr unsigned char Tracer[] = {
#include "GeometrySurface.wgsl.h"
};

// Mirrors `Params` in GeometrySurface.wgsl.
struct Params {
    glm::vec4 boxMin, boxMax;
    glm::vec4 regionColor, wallColor;
    uint32_t cullWallSide, maxSteps, pad0, pad1;
};
static_assert(sizeof(Params) == 80);

constexpr std::array<Wgpu::LayoutEntry, 2> groupEntries{{
    {.binding = 0, .type = Wgpu::BindingType::ReadOnlyStorage, .visibility = Wgpu::ShaderStage::Fragment},
    {.binding = 1, .type = Wgpu::BindingType::Uniform, .visibility = Wgpu::ShaderStage::Fragment},
}};


}

/// Group 1 over the current program buffer: recreated when the program buffer changes.
struct GeometrySurface::Group {
    Group(Wgpu::Device& device, Wgpu::BindingLayout& layout, std::span<const Wgpu::Binding> bindings)
        : set(device, layout, bindings) {}
    Wgpu::BindingSet set;
};

GeometrySurface::GeometrySurface(RenderLayout& layout, const Boundaries::Wgpu::GpuGeometry& geometry,
                                 const Boundaries::Space& space, Boundaries::Wgpu::DistanceSource distance)
    : geometry_(geometry),
      space_(space),
      distance_(distance),
      renderLayout_(layout),
      device_(layout.device()),
      layout_(device_, groupEntries, "geometry-surface"),
      params_(device_, {.size = sizeof(Params),
          .usage = Wgpu::BufferUsage::Uniform | Wgpu::BufferUsage::CopyDestination, .label = "geometry-surface/params"}) {
    build();
}

void GeometrySurface::build() {
    structure_ = Boundaries::Wgpu::structure(geometry_.program());
    pipeline_.reset();
    shader_ = std::make_unique<Wgpu::Shader>(device_, Wgpu::ShaderDesc{
        .source = std::string(Boundaries::Wgpu::geometrySource()) + distance_(geometry_.program(), "traceDistance") +
            std::string(Wgpu::shaderText(Tracer)),
        .label = "geometry-surface"});
    pipeline_ = std::make_unique<RenderPipeline>(device_, *shader_, RenderPipelineDesc{
        .colorFormat = renderLayout_.colorFormat(), .depthFormat = renderLayout_.depthFormat(),
        .layouts = {&renderLayout_.camera(), &layout_}, .label = "geometry-surface"});
}

GeometrySurface::~GeometrySurface() = default;

void GeometrySurface::render(RenderPass& pass, const ViewState& view) {
    // A new structure needs new code; new parameters are only data in the program buffer.
    if (Boundaries::Wgpu::structure(geometry_.program()) != structure_)
        build();
    if (!group_ || geometry_.generation() != generation_) {
        const std::array bindings{
            Wgpu::Binding{.binding = 0, .buffer = &geometry_.code()},
            Wgpu::Binding{.binding = 1, .buffer = &params_},
        };
        group_.reset();
        group_ = std::make_unique<Group>(device_, layout_, bindings);
        generation_ = geometry_.generation();
    }
    const Params params{
        .boxMin = glm::vec4(space_.origin(), 0.0f),
        .boxMax = glm::vec4(space_.origin() + space_.size(), 0.0f),
        .regionColor = regionColor,
        .wallColor = wallColor,
        .cullWallSide = cullWallSide ? 1u : 0u,
        .maxSteps = maxSteps,
        .pad0 = 0,
        .pad1 = 0,
    };
    device_.writeBuffer(params_, 0, std::as_bytes(std::span(&params, 1)));
    pass.setPipeline(*pipeline_);
    pass.setBindings(0, view.cameraGroup);
    pass.setBindings(1, group_->set);
    pass.draw(3);
}
