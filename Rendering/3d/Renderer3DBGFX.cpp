#include "Renderer3DBGFX.h"

#include <glm/gtc/matrix_transform.hpp>

Renderer3DBGFX::Renderer3DBGFX(sf::RenderTarget& t, sf::View& gv, SimBox& simBox) : RendererBGFX(t, gv, simBox) {
    camera.setMode(Camera::Mode::Orbit);

    atomProgram = loadProgram("assets/shaders/bgfx/atom3d.v.bin", "assets/shaders/bgfx/atom3d.f.bin");
    bondProgram = loadProgram("assets/shaders/bgfx/bond.v.bin", "assets/shaders/bgfx/bond.f.bin");
    boxProgram = loadProgram("assets/shaders/bgfx/box.v.bin", "assets/shaders/bgfx/box.f.bin");
    gridProgram = loadProgram("assets/shaders/bgfx/grid.v.bin", "assets/shaders/bgfx/grid.f.bin");

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
