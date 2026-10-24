#include "Viewport.hpp"

#include <array>
#include <span>

namespace {

Wgpu::Binding cameraBinding(Wgpu::Buffer& buffer) {
    return {.binding = 0, .buffer = &buffer, .size = sizeof(CameraUniform)};
}

}

Viewport::Viewport(RenderLayout& layout, Camera& camera)
    : device_(layout.device()),
      camera_(camera),
      uniform_(device_, {.size = sizeof(CameraUniform),
          .usage = Wgpu::BufferUsage::Uniform | Wgpu::BufferUsage::CopyDestination,
          .label = "viewport/camera"}),
      group_(device_, layout.camera(), std::span<const Wgpu::Binding>{std::array{cameraBinding(uniform_)}}) {}

void Viewport::render(RenderPass& pass, glm::uvec2 surfaceSize) {
    Rect rect{};
    if (!resolveRect(surfaceSize, rect))
        return;

    const float aspect = float(rect.width) / float(rect.height);
    const CameraUniform camera{
        .viewProjection = camera_.viewProjection(aspect),
        .view = camera_.viewMatrix(),
        .projection = camera_.projectionMatrix(aspect),
        .position = glm::vec4(camera_.transform().position, 1.0f),
    };
    device_.writeBuffer(uniform_, 0, std::as_bytes(std::span(&camera, 1)));

    pass.setViewport(rect);
    pass.setScissor(rect);
    const ViewState state{.rect = rect, .camera = camera, .cameraGroup = group_};
    for (SceneObject* object : objects_)
        object->render(pass, state);
}
