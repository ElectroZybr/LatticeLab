#pragma once

#include "App/interaction/tools/ITool.h"

class SpawnBoxTool final : public ITool {
public:
    explicit SpawnBoxTool(ToolContext& context) noexcept;

    void onLeftPressed(glm::ivec2 mousePos) override;
    void onLeftReleased(glm::ivec2 mousePos) override;
    void onFrame(glm::ivec2 mousePos, float deltaTime) override;
    void reset() override;

private:
    glm::vec3 startLocalWorld_ = glm::vec3(0.0f);
};
