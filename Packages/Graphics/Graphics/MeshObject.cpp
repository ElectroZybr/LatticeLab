#include "MeshObject.hpp"

#include <cmath>
#include <span>

#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <Wgpu/ShaderSource.hpp>

namespace {

constexpr unsigned char Shader[] = {
#include "Mesh.wgsl.h"
};

struct Uniform {
    glm::mat4 model;
    glm::mat4 normalMatrix;
    glm::vec4 color;
    glm::vec4 fill;
    glm::vec4 wire;
    glm::vec4 edge;
    glm::vec4 hidden;
    glm::vec4 material;
};
static_assert(sizeof(Uniform) == 224);

constexpr std::array<Wgpu::LayoutEntry, 4> groupEntries{{
    {.binding = 0, .type = Wgpu::BindingType::ReadOnlyStorage, .visibility = Wgpu::ShaderStage::Vertex},
    {.binding = 1, .type = Wgpu::BindingType::ReadOnlyStorage, .visibility = Wgpu::ShaderStage::Vertex},
    {.binding = 2, .type = Wgpu::BindingType::Uniform},
    {.binding = 3, .type = Wgpu::BindingType::ReadOnlyStorage, .visibility = Wgpu::ShaderStage::Vertex},
}};

}

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
      noEdges_(device_, {.size = sizeof(MeshEdge), .usage = Wgpu::BufferUsage::Storage, .label = "mesh/no-edges"}),
      shader_(device_, {.source = std::string(Wgpu::shaderText(Shader)), .label = "mesh"}) {
    faces(false);
}

MeshObject::~MeshObject() = default;

MeshObject::Mode MeshObject::mode() const {
    if (display == "shaded-edges") return Mode::ShadedEdges;
    if (display == "hidden-removed") return Mode::HiddenRemoved;
    if (display == "hidden-visible") return Mode::HiddenVisible;
    if (display == "wireframe") return Mode::Wireframe;
    return Mode::Shaded;
}

bool MeshObject::transparent() const {
    const Mode m = mode();
    return m == Mode::Wireframe || ((m == Mode::Shaded || m == Mode::ShadedEdges) && opacity < 1.0f);
}

RenderPipeline& MeshObject::faces(bool blend) {
    auto& pipeline = faces_[(blend ? 2 : 0) + (cullBack ? 1 : 0)];
    if (!pipeline)
        pipeline = std::make_unique<RenderPipeline>(device_, shader_, RenderPipelineDesc{
            .colorFormat = layout_.colorFormat(), .depthFormat = layout_.depthFormat(),
            .depthWrite = !blend, .blend = blend, .cull = cullBack ? CullMode::Back : CullMode::None,
            .layouts = {&layout_.camera(), &groupLayout_}, .label = blend ? "mesh/transparent" : "mesh"});
    return *pipeline;
}

RenderPipeline& MeshObject::lines(Lines kind) {
    auto& pipeline = lines_[size_t(kind)];
    if (!pipeline) {
        const bool hidden = kind == Lines::Hidden;
        pipeline = std::make_unique<RenderPipeline>(device_, shader_, RenderPipelineDesc{
            .colorFormat = layout_.colorFormat(), .depthFormat = layout_.depthFormat(), .depthWrite = false,
            .depthCompare = hidden ? DepthCompare::Greater
                : kind == Lines::Always ? DepthCompare::Always : DepthCompare::LessEqual,
            .blend = true, .layouts = {&layout_.camera(), &groupLayout_},
            .vertexEntry = hidden ? "vsHidden" : "vsEdge", .fragmentEntry = hidden ? "fsHidden" : "fsEdge",
            .label = hidden ? "mesh/hidden-lines" : "mesh/lines"});
    }
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
            Wgpu::Binding{.binding = 3, .buffer = mesh_.edges() ? mesh_.edges() : &noEdges_},
        };
        group_.reset();
        group_ = std::make_unique<Group>(device_, groupLayout_, bindings);
        generation_ = mesh_.generation();
    }

    const Mode m = mode();
    const bool lit = m == Mode::Shaded || m == Mode::ShadedEdges;
    const glm::mat4 model = SceneObject::model();
    const Uniform uniform{
        .model = model,
        .normalMatrix = glm::transpose(glm::inverse(model)),
        .color = glm::vec4(color, lit ? opacity : 1.0f),
        .fill = glm::vec4(fillColor, lit ? 1.0f : 0.0f),
        .wire = glm::vec4(triangleColor, triangles ? triangleWidth : 0.0f),
        .edge = glm::vec4(edgeColor, edgeWidth),
        .hidden = glm::vec4(hiddenColor, hiddenWidth),
        .material = glm::vec4(specular, shininess, std::cos(glm::radians(creaseAngle)), dashLength),
    };
    device_.writeBuffer(uniform_, 0, std::as_bytes(std::span(&uniform, 1)));
    pass.setBindings(0, view.cameraGroup);
    pass.setBindings(1, group_->set);

    if (m != Mode::Wireframe) {
        pass.setPipeline(faces(lit && opacity < 1.0f));
        pass.draw(mesh_.indexCount());
    }
    const uint32_t lineVertices = mesh_.edgeCount() * 6;
    if (m == Mode::Shaded || !lineVertices)
        return;
    if (m == Mode::HiddenVisible) {
        pass.setPipeline(lines(Lines::Hidden));
        pass.draw(lineVertices);
    }
    pass.setPipeline(lines(m == Mode::Wireframe ? Lines::Always : Lines::Visible));
    pass.draw(lineVertices);
}
