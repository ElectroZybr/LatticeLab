#include "MeshObject.hpp"

#include <span>

#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <Wgpu/ShaderSource.hpp>

namespace {

constexpr unsigned char Shader[] = {
#include "Mesh.wgsl.h"
};

// Mirrors `Object` in Mesh.wgsl.
struct Uniform {
    glm::mat4 model;
    glm::mat4 normalMatrix;
    glm::vec4 color;
    glm::vec4 wire;
    glm::vec4 material;
    glm::uvec4 flags;
};
static_assert(sizeof(Uniform) == 192);

constexpr std::array<Wgpu::LayoutEntry, 3> groupEntries{{
    {.binding = 0, .type = Wgpu::BindingType::ReadOnlyStorage, .visibility = Wgpu::ShaderStage::Vertex},
    {.binding = 1, .type = Wgpu::BindingType::ReadOnlyStorage, .visibility = Wgpu::ShaderStage::Vertex},
    {.binding = 2, .type = Wgpu::BindingType::Uniform},
}};

}

/// Group 1 over the mesh buffers: recreated when they change.
struct MeshObject::Group {
    Group(Wgpu::Device& device, Wgpu::BindingLayout& layout, std::span<const Wgpu::Binding> bindings)
        : set(device, layout, bindings) {}
    Wgpu::BindingSet set;
};

MeshObject::MeshObject(RenderLayout& layout, GpuMesh& mesh)
    : layout_(layout),
      mesh_(mesh),
      device_(layout.device()),
      groupLayout_(device_, groupEntries, "mesh"),
      uniform_(device_, {.size = sizeof(Uniform),
          .usage = Wgpu::BufferUsage::Uniform | Wgpu::BufferUsage::CopyDestination, .label = "mesh/object"}),
      shader_(device_, {.source = std::string(Wgpu::shaderText(Shader)), .label = "mesh"}) {
    pipeline();
}

MeshObject::~MeshObject() = default;

RenderPipeline& MeshObject::pipeline() {
    const bool blend = transparent();
    auto& pipeline = pipelines_[(blend ? 2 : 0) + (cullBack ? 1 : 0)];
    if (!pipeline)
        pipeline = std::make_unique<RenderPipeline>(device_, shader_, RenderPipelineDesc{
            .colorFormat = layout_.colorFormat(), .depthFormat = layout_.depthFormat(),
            // Transparent faces test the depth of opaque ones but do not hide each other.
            .depthWrite = !blend, .blend = blend, .cull = cullBack ? CullMode::Back : CullMode::None,
            .layouts = {&layout_.camera(), &groupLayout_}, .label = blend ? "mesh/transparent" : "mesh"});
    return *pipeline;
}

void MeshObject::render(RenderPass& pass, const ViewState& view) {
    if (!mesh_.indexCount())
        return;
    if (!group_ || mesh_.generation() != generation_) {
        const std::array bindings{
            Wgpu::Binding{.binding = 0, .buffer = mesh_.vertices()},
            Wgpu::Binding{.binding = 1, .buffer = mesh_.indices()},
            Wgpu::Binding{.binding = 2, .buffer = &uniform_},
        };
        group_.reset();
        group_ = std::make_unique<Group>(device_, groupLayout_, bindings);
        generation_ = mesh_.generation();
    }
    const glm::mat4 model = SceneObject::model();
    const Uniform uniform{
        .model = model,
        .normalMatrix = glm::transpose(glm::inverse(model)),
        .color = glm::vec4(color, opacity),
        .wire = glm::vec4(wireColor, wireWidth),
        .material = glm::vec4(specular, shininess, 0.0f, 0.0f),
        .flags = glm::uvec4(solid ? 1u : 0u, wireframe ? 1u : 0u, 0u, 0u),
    };
    device_.writeBuffer(uniform_, 0, std::as_bytes(std::span(&uniform, 1)));
    pass.setPipeline(pipeline());
    pass.setBindings(0, view.cameraGroup);
    pass.setBindings(1, group_->set);
    pass.draw(mesh_.indexCount());
}
