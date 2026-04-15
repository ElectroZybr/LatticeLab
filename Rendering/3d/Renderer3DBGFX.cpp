#include "Renderer3DBGFX.h"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "generated/shaders/shader_registry.h"

Renderer3DBGFX::Renderer3DBGFX(SimBox& simBox) : RendererBGFX(simBox) {
    atomProgram = loadEmbeddedProgram(s_allShaders, "atom3d");
    bondProgram = loadEmbeddedProgram(s_allShaders, "bond");
    boxProgram = loadEmbeddedProgram(s_allShaders, "box");
    gridProgram = loadEmbeddedProgram(s_allShaders, "grid");

    camera.setMode(Camera::Mode::Orbit);
    camera.resetView();
}

void Renderer3DBGFX::updateMatrices() {
    const float aspect = static_cast<float>(camera.screenSize.x) / static_cast<float>(camera.screenSize.y);
    projection = glm::perspective(glm::radians(45.f), aspect, 0.1f, 1000.f);
    view = camera.getViewMatrix();
}

glm::vec3 Renderer3DBGFX::getLightDir() {
    const glm::vec3 eye = camera.getEyePosition();
    return glm::normalize(glm::vec3(view * glm::vec4(eye, 0.f)) + glm::vec3(25.f, 25.f, 0.f));
}
