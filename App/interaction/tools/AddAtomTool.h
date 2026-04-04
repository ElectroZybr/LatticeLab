#pragma once

#include "App/interaction/tools/ITool.h"

class AddAtomTool final : public ITool {
public:
    explicit AddAtomTool(ToolContext& context) noexcept;

    void onLeftPressed(sf::Vector2i mousePos) override;
};
