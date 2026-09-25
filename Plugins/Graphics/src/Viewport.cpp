#include "Viewport.hpp"
#include "TransformMode.hpp"

Viewport::Viewport(NodeBuild node) {
    auto camera = node.addSlot<Camera>("MainCamera");
    camera.choice<Camera>();
    camera_ = Ref<Camera>{camera.get()};
    camera_->setPosition({0.0f, 0.0f, 2.0f});
    auto controller = node.addSlot<TransformController>("Camera");
    controller.choice<FreeCameraController>();
    node.param("cursor", cursor_);
}

void Viewport::configure(NodeBuildView node) {
    deviceFocus_.emplace(node.focus<GPU::Device>());
    device_ = Ref<GPU::Device>{deviceFocus_->get()};
    if (!device_) return;

    GPU::BufferDesc desc{};
    desc.size = sizeof(glm::mat4);
    desc.usage = GPU::BufferUsage::Uniform | GPU::BufferUsage::CopyDestination;
    uniform_ = device_->createBuffer(desc);
}
