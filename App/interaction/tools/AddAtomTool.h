#pragma once

#include "App/interaction/tools/ITool.h"
#include "Lattice/Engine/physics/Atom/AtomStorage.h"

class AddAtomTool final : public ITool {
public:
    explicit AddAtomTool(ToolContext& context) noexcept;

    void onLeftPressed(glm::ivec2 mousePos) override;
    void onLeftReleased(glm::ivec2 mousePos) override;
    void reset() override;

private:
    static constexpr AtomStorage::AtomId InvalidAtomId = AtomStorage::InvalidAtomId;

    AtomStorage::AtomId launchedAtomId_ = InvalidAtomId;
    glm::vec3 launchOrigin_ = glm::vec3(0.0f);
};
