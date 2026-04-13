#include "Renderer3DBGFX.h"

#include <glm/gtc/matrix_transform.hpp>

#include "shaders/shader_registry.h"

Renderer3DBGFX::Renderer3DBGFX(sf::RenderTarget& t, sf::View& gv, SimBox& simBox) : RendererBGFX(t, gv, simBox) {
    camera.setMode(Camera::Mode::Orbit);

    atomProgram = loadEmbeddedProgram(s_allShaders, "atom3d");
    bondProgram = loadEmbeddedProgram(s_allShaders, "bond");
    boxProgram = loadEmbeddedProgram(s_allShaders, "box");
    gridProgram = loadEmbeddedProgram(s_allShaders, "grid");

    camera.freePosition = simBox.size / 2.f;
    camera.freePosition.z = -200.f;
    camera.setZoom(std::max({simBox.size.x, simBox.size.y, simBox.size.z}) * 0.02);
}

void Renderer3DBGFX::updateMatrices() {
    const auto size = target.getSize();
    const float aspect = static_cast<float>(size.x) / static_cast<float>(size.y);

    projection = glm::perspective(glm::radians(45.f), aspect, 0.1f, 1000.f);
    view = camera.getViewMatrix();
}

glm::vec3 Renderer3DBGFX::getLightDir() {
    const glm::vec3 eye = camera.getEyePosition();
    return glm::normalize(glm::vec3(view * glm::vec4(eye, 0.f)) + glm::vec3(25.f, 25.f, 0.f));
}
