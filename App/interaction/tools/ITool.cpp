#include "ITool.h"

#include "Rendering/BaseRenderer.h"

ITool::ITool(ToolContext& context) noexcept
    : context_(context) {}

ITool::~ITool() = default;

void ITool::onLeftPressed(sf::Vector2i mousePos) {
    (void)mousePos;
}

void ITool::onLeftReleased(sf::Vector2i mousePos) {
    (void)mousePos;
}

bool ITool::onRightPressed(sf::Vector2i mousePos) {
    (void)mousePos;
    return false;
}

void ITool::onFrame(sf::Vector2i mousePos, float deltaTime) {
    (void)mousePos;
    (void)deltaTime;
}

void ITool::reset() {}

Vec3f ITool::screenToWorld(sf::Vector2i mousePos) const {
    if (IRenderer* renderer = context_.activeRenderer(); renderer != nullptr) {
        return renderer->camera.screenToWorld(mousePos);
    }
    return {};
}

sf::Vector2i ITool::worldToScreen(Vec3f worldPos) const {
    if (IRenderer* renderer = context_.activeRenderer(); renderer != nullptr) {
        return renderer->camera.worldToScreen(worldPos);
    }
    return {};
}
