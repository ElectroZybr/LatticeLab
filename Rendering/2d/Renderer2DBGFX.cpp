#include "Renderer2DBGFX.h"

#include <glm/gtc/matrix_transform.hpp>

#include "generated/shaders/shader_registry.h"

Renderer2DBGFX::Renderer2DBGFX(GLFWwindow* window, SimBox& simBox) : RendererBGFX(window, simBox) {
    camera.position = Vec2f(simBox.size.x, simBox.size.y) / 2.f;
    camera.setZoom(std::max(simBox.size.x, simBox.size.y) * 0.07);

    atomProgram = loadEmbeddedProgram(s_allShaders, "atom2d");
    bondProgram = loadEmbeddedProgram(s_allShaders, "bond");
    boxProgram = loadEmbeddedProgram(s_allShaders, "box");
    gridProgram = loadEmbeddedProgram(s_allShaders, "grid");
}

void Renderer2DBGFX::updateMatrices() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    const float viewWidth = static_cast<float>(width) / camera.getZoom();
    const float viewHeight = viewWidth / aspect;

    projection = glm::ortho(-viewWidth / 2.f, viewWidth / 2.f, -viewHeight / 2.f, viewHeight / 2.f, -10000.f, 10000.f);
    view = glm::translate(glm::mat4(1.f), glm::vec3(-camera.getPosition().x, -camera.getPosition().y, 0.f));
}