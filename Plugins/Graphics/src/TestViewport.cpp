#include "TestViewport.hpp"

#include <fstream>
#include <iterator>
#include <span>

#include <glm/gtc/type_ptr.hpp>
#include <Lattice/Kernel/Exception.hpp>

#include "Camera.hpp"
#include "TransformController.hpp"
#include "TransformMode.hpp"

namespace {

std::string loadTestShader() {
    std::ifstream file("Plugins/Graphics/src/shaders/circle.wgsl");
    if (!file)
        throw Lattice::Exception("TestViewport", "cannot read circle.wgsl");
    return {std::istreambuf_iterator<char>(file), {}};
}

}

TestViewport::TestViewport(NodeBuild node, const Desc& desc) {
    auto camera = node.addSlot<Camera>("MainCamera");
    camera.choice<Camera>();
    camera_ = Ref<Camera>{camera.get()};
    camera_->setPosition({0.0f, 0.0f, 2.0f});

    auto controller = node.addSlot<TransformController>("Camera");
    controller.choice<FreeCameraController>();
    controller_ = Ref<TransformController>{controller.get()};
    node.param("cursor", cursorValue());

    auto mounted = node.mount<GPU::Device>();
    device_ = mounted.ref();

    GPU::BufferDesc bufferDesc{};
    bufferDesc.size = sizeof(glm::mat4);
    bufferDesc.usage = GPU::BufferUsage::Uniform | GPU::BufferUsage::CopyDestination;
    uniform_ = mounted.addLocal<GPU::Buffer>("camera", bufferDesc);

    GPU::ShaderDesc shaderDesc{};
    shaderDesc.source = loadTestShader();
    shader_ = mounted.addLocal<GPU::Shader>("shader", shaderDesc);

    GPU::PipelineDesc pipelineDesc{};
    pipelineDesc.shader = shader_.get();
    pipelineDesc.colorFormat = desc.colorFormat;
    pipeline_ = mounted.addLocal<GPU::Pipeline>("pipeline", pipelineDesc);

    GPU::Binding binding{};
    binding.binding = 0;
    binding.buffer = uniform_.get();
    binding.size = sizeof(glm::mat4);
    bindings_ = device_->createBindingSet(*pipeline_, 0, std::span(&binding, 1));
}

void TestViewport::render(GPU::RenderPass& pass, glm::uvec2 surfaceSize) {
    if (!device_ || !uniform_ || !pipeline_ || !camera_ || !bindings_)
        return;

    GPU::Rect rect{};
    if (!resolveRect(surfaceSize, rect))
        return;

    if (controller_)
        controller_->update();

    const auto matrix = camera_->viewProjection(float(rect.width) / float(rect.height));
    device_->writeBuffer(*uniform_, 0, std::as_bytes(std::span(&matrix, 1)));

    pass.setViewport(rect);
    pass.setScissor(rect);
    pass.setPipeline(*pipeline_);
    pass.setBindings(0, *bindings_);
    pass.draw(6);
}
