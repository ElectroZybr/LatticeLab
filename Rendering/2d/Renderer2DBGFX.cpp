#include "Renderer2DBGFX.h"

#include <glm/gtc/matrix_transform.hpp>

#include "generated/shaders/shader_registry.h"

Renderer2DBGFX::Renderer2DBGFX(GLFWwindow* window, SimBox& simBox) : RendererBGFX(window, simBox) {
    atomProgram = loadEmbeddedProgram(s_allShaders, "atom2d");
    bondProgram = loadEmbeddedProgram(s_allShaders, "bond");
    boxProgram = loadEmbeddedProgram(s_allShaders, "box");
    gridProgram = loadEmbeddedProgram(s_allShaders, "grid");

    camera.setMode(Camera::Mode::Mode2D);
    camera.resetView();
}

void Renderer2DBGFX::updateMatrices() {
    const float aspect = static_cast<float>(camera.screenSize.x) / static_cast<float>(camera.screenSize.y);
    const float viewWidth = static_cast<float>(camera.screenSize.x) / camera.getZoom();
    const float viewHeight = viewWidth / aspect;

    projection = glm::ortho(-viewWidth / 2.f, viewWidth / 2.f, -viewHeight / 2.f, viewHeight / 2.f, -10000.f, 10000.f);
    view = glm::translate(glm::mat4(1.f), glm::vec3(-camera.getPosition().x, -camera.getPosition().y, 0.f));
}