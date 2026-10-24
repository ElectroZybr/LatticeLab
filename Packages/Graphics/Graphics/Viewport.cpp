#include "Viewport.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <span>

Viewport::Viewport(RenderLayout& layout, Camera& camera)
    : layout_(layout),
      device_(layout.device()),
      camera_(camera),
      uniform_(device_, {.size = sizeof(CameraUniform),
          .usage = Wgpu::BufferUsage::Uniform | Wgpu::BufferUsage::CopyDestination,
          .label = "viewport/camera"}) {
    uploadLights();
}

void Viewport::uploadLights() {
    // The buffer grows with the lights (never below 4); group 0 follows it.
    const size_t capacity = std::max<size_t>(4, lights_.size());
    if (capacity > lightCapacity_) {
        lightCapacity_ = capacity;
        lightBuffer_ = std::make_unique<Wgpu::Buffer>(device_, Wgpu::BufferDesc{
            .size = sizeof(LightsHeader) + capacity * sizeof(LightRecord),
            .usage = Wgpu::BufferUsage::Storage | Wgpu::BufferUsage::CopyDestination, .label = "viewport/lights"});
        const std::array bindings{
            Wgpu::Binding{.binding = 0, .buffer = &uniform_, .size = sizeof(CameraUniform)},
            Wgpu::Binding{.binding = 1, .buffer = lightBuffer_.get()},
        };
        group_.reset();
        group_ = std::make_unique<Wgpu::BindingSet>(device_, layout_.camera(), bindings);
    }
    const LightsHeader header{.ambient = glm::vec4(ambient, 0.0f), .count = glm::uvec4(uint32_t(lights_.size()), 0, 0, 0)};
    lightBytes_.resize(sizeof(LightsHeader) + lights_.size() * sizeof(LightRecord));
    std::memcpy(lightBytes_.data(), &header, sizeof(header));
    for (size_t i = 0; i < lights_.size(); ++i) {
        const Light& light = *lights_[i];
        const LightRecord record{
            .position = light.point ? glm::vec4(light.position, 1.0f) : glm::vec4(glm::normalize(light.direction), 0.0f),
            .color = glm::vec4(light.color * light.intensity, light.range),
        };
        std::memcpy(lightBytes_.data() + sizeof(LightsHeader) + i * sizeof(LightRecord), &record, sizeof(record));
    }
    device_.writeBuffer(*lightBuffer_, 0, std::span<const std::byte>(lightBytes_));
}

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
    uploadLights();

    pass.setViewport(rect);
    pass.setScissor(rect);
    const ViewState state{.rect = rect, .camera = camera, .cameraGroup = *group_};
    for (bool transparent : {false, true})
        for (SceneObject* object : objects_)
            if (object->transparent() == transparent)
                object->render(pass, state);
}
