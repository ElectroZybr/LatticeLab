#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "Renderer2DWGPU.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Rendering/RendererWGPU.h"
// #include "generated/shaders/shader_registry.h"

Renderer2DWGPU::Renderer2DWGPU(SimBox& simBox, wgpu::Device device, wgpu::TextureFormat surfaceFormat)
    : RendererWGPU(simBox, device, surfaceFormat) {
    // atomProgram = loadEmbeddedProgram(s_allShaders, "atom2d");
    // bondProgram = loadEmbeddedProgram(s_allShaders, "bond");
    // boxProgram = loadEmbeddedProgram(s_allShaders, "box");
    // gridProgram = loadEmbeddedProgram(s_allShaders, "grid");

    camera.setMode(Camera::Mode::Mode2D);
    camera.resetView();
}

void Renderer2DWGPU::updateMatrices() {
    const float aspect = static_cast<float>(camera.screenSize.x) / static_cast<float>(camera.screenSize.y);
    const float viewWidth = static_cast<float>(camera.screenSize.x) / camera.getZoom();
    const float viewHeight = viewWidth / aspect;

    projection = glm::ortho(-viewWidth / 2.f, viewWidth / 2.f, -viewHeight / 2.f, viewHeight / 2.f, -10000.f, 10000.f);
    view = glm::translate(glm::mat4(1.f), glm::vec3(-camera.getPosition().x, -camera.getPosition().y, 0.f));
}