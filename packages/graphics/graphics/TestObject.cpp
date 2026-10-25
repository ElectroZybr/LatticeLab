#include "TestObject.hpp"

#include <array>
#include <fstream>
#include <iterator>
#include <span>

#include <Lattice/tools/Exception.hpp>

namespace {

using Error = Lattice::Exception<TestObject>;

std::string readShader(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file)
        throw Error("cannot read shader '{}'", path.string());
    return {std::istreambuf_iterator<char>(file), {}};
}

constexpr std::array<Wgpu::LayoutEntry, 1> modelEntries{{{.binding = 0, .type = Wgpu::BindingType::Uniform,
    .visibility = Wgpu::ShaderStage::Vertex}}};

Wgpu::Binding modelBinding(Wgpu::Buffer& buffer) {
    return {.binding = 0, .buffer = &buffer, .size = sizeof(glm::mat4)};
}

}

TestObject::TestObject(RenderLayout& layout, const std::filesystem::path& shader)
    : device_(layout.device()),
      modelLayout_(device_, modelEntries, "test/model"),
      model_(device_, {.size = sizeof(glm::mat4),
          .usage = Wgpu::BufferUsage::Uniform | Wgpu::BufferUsage::CopyDestination, .label = "test/model"}),
      modelGroup_(device_, modelLayout_, std::span<const Wgpu::Binding>{std::array{modelBinding(model_)}}),
      shader_(device_, {.source = readShader(shader), .label = shader.filename().string()}),
      pipeline_(device_, shader_, {.colorFormat = layout.colorFormat(), .depthFormat = layout.depthFormat(),
          .layouts = {&layout.camera(), &modelLayout_}, .label = "test"}) {}

void TestObject::render(RenderPass& pass, const ViewState& view) {
    const glm::mat4 matrix = model();
    device_.writeBuffer(model_, 0, std::as_bytes(std::span(&matrix, 1)));
    pass.setPipeline(pipeline_);
    pass.setBindings(0, view.cameraGroup);
    pass.setBindings(1, modelGroup_);
    pass.draw(6);
}
