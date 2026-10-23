#include "TestViewport.hpp"

#include <array>
#include <fstream>
#include <iterator>
#include <span>

#include <glm/gtc/type_ptr.hpp>
#include <Lattice/Tools/Exception.hpp>

namespace {

std::string readShader(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file)
        throw Lattice::Exception<TestViewport>("cannot read shader '{}'", path.string());
    return {std::istreambuf_iterator<char>(file), {}};
}

WGPU::Binding uniformBinding(WGPU::Buffer& buffer) {
    return {.binding = 0, .buffer = &buffer, .size = sizeof(glm::mat4)};
}

}

TestViewport::TestViewport(WGPU::Device& device, WGPU::TextureFormat colorFormat, Camera& camera,
                           const std::filesystem::path& shader)
    : device_(device),
      camera_(camera),
      uniform_(device, {.size = sizeof(glm::mat4),
          .usage = WGPU::BufferUsage::Uniform | WGPU::BufferUsage::CopyDestination,
          .label = "test/uniform"}),
      shader_(device, {.source = readShader(shader), .label = shader.filename().string()}),
      pipeline_(device, shader_, {.colorFormat = colorFormat, .label = "test"}),
      bindings_(device, pipeline_, 0, std::span<const WGPU::Binding>{std::array{uniformBinding(uniform_)}}) {}

void TestViewport::render(WGPU::RenderPass& pass, glm::uvec2 surfaceSize) {
    WGPU::Rect rect{};
    if (!resolveRect(surfaceSize, rect))
        return;

    const auto matrix = camera_.viewProjection(float(rect.width) / float(rect.height));
    device_.writeBuffer(uniform_, 0, std::as_bytes(std::span(&matrix, 1)));

    pass.setViewport(rect);
    pass.setScissor(rect);
    pass.setPipeline(pipeline_);
    pass.setBindings(0, bindings_);
    pass.draw(6);
}
