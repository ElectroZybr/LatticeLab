#include "Renderer2DBGFX.h"

#include <glm/gtc/matrix_transform.hpp>

Renderer2DBGFX::Renderer2DBGFX(sf::RenderTarget& t, sf::View& gv, SimBox& simBox) : RendererBGFX(t, gv, simBox) {
    camera.position = Vec2f(simBox.size.x, simBox.size.y) / 2.f;
    camera.setZoom(std::max(simBox.size.x, simBox.size.y) * 0.07);
}

void Renderer2DBGFX::updateMatrices() {
    const auto size = target.getSize();
    if (size.x == 0 || size.y == 0) {
        return;
    }

    const float aspect = float(size.x) / float(size.y);
    const float viewWidth = float(size.x) / camera.getZoom();
    const float viewHeight = viewWidth / aspect;

    projection = glm::ortho(-viewWidth / 2.f, viewWidth / 2.f, -viewHeight / 2.f, viewHeight / 2.f, -10000.f, 10000.f);
    view = glm::translate(glm::mat4(1.f), glm::vec3(-camera.getPosition().x, -camera.getPosition().y, 0.f));
}