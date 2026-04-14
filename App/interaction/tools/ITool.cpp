#include "ITool.h"

#include "Rendering/BaseRenderer.h"

ITool::ITool(ToolContext& context) noexcept : context_(context) {}

ITool::~ITool() = default;

void ITool::onLeftPressed(Vec2u mousePos) { (void)mousePos; }

void ITool::onLeftReleased(Vec2u mousePos) { (void)mousePos; }

bool ITool::onRightPressed(Vec2u mousePos) {
    (void)mousePos;
    return false;
}

void ITool::onFrame(Vec2u mousePos, float deltaTime) {
    (void)mousePos;
    (void)deltaTime;
}

void ITool::reset() {}

Vec3f ITool::screenToWorld(Vec2u mousePos) const {
    if (IRenderer* renderer = context_.activeRenderer(); renderer != nullptr) {
        return renderer->camera.screenToWorld(mousePos);
    }
    return {};
}

Vec2u ITool::worldToScreen(Vec3f worldPos) const {
    if (IRenderer* renderer = context_.activeRenderer(); renderer != nullptr) {
        return renderer->camera.worldToScreen(worldPos);
    }
    return {};
}
