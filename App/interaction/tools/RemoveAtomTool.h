#pragma once

#include "App/interaction/tools/ITool.h"

class RemoveAtomTool final : public ITool {
public:
    explicit RemoveAtomTool(ToolContext& context) noexcept;

    void onLeftPressed(Vec2i mousePos) override;
};
