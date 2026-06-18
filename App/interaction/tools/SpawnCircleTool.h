#pragma once

#include "App/interaction/tools/ITool.h"

class SpawnCircleTool final : public ITool {
public:
    explicit SpawnCircleTool(ToolContext& context) noexcept;

    void onLeftPressed(glm::ivec2 mousePos) override;
    void onLeftReleased(glm::ivec2 mousePos) override;
    void onFrame(glm::ivec2 mousePos, float deltaTime) override;
    void reset() override;

private:
    glm::vec3 centerLocalWorld_ = glm::vec3(0.0f);
};
