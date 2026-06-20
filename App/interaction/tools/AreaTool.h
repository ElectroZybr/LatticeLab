#pragma once

#include <limits>
#include <vector>

#include "App/AppSignals.h"
#include "App/interaction/tools/ITool.h"

class SideToolsPanel;

class AreaTool final : public ITool {
public:
    AreaTool(ToolContext& context, SideToolsPanel& sideToolsPanel) noexcept;

    void onLeftPressed(glm::ivec2 mousePos) override;
    void onLeftReleased(glm::ivec2 mousePos) override;
    void onFrame(glm::ivec2 mousePos, float deltaTime) override;
    void reset() override;

private:
    void beginRect(glm::ivec2 mousePos);
    void beginLasso(glm::ivec2 mousePos);
    void beginBrush(glm::ivec2 mousePos);
    void finishRect(glm::ivec2 mousePos);
    void finishLasso(glm::ivec2 mousePos);
    void finishBrush(glm::ivec2 mousePos);
    void updateRect(glm::ivec2 mousePos);
    void updateLasso(glm::ivec2 mousePos);
    void updateBrush(glm::ivec2 mousePos, bool uiHovered);
    void applyBrushAt(glm::ivec2 mousePos);
    float brushRadiusScreen(glm::ivec2 mousePos) const;
    AppSignals::UI::GeneratorRegionSpec makeRectRegion(glm::ivec2 mousePos) const;
    AppSignals::UI::GeneratorRegionSpec makeLassoRegion() const;
    AppSignals::UI::GeneratorRegionSpec makeBrushRegion(glm::ivec2 mousePos) const;
    void updateSelectionCount() const;
    bool cumulativeSelection() const;

    SideToolsPanel& sideToolsPanel_;
    glm::vec3 rectStartLocalWorld_ = glm::vec3(0.0f);
    bool brushActive_ = false;
    bool brushSelectionActive_ = false;
    bool brushSelectionCumulative_ = false;
    glm::ivec2 lastBrushSample_ = glm::ivec2(std::numeric_limits<int>::min());
};
