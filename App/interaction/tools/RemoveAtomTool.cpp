#include "RemoveAtomTool.h"

#include <algorithm>
#include <vector>

#include "App/interaction/picking/PickingSystem.h"
#include "Engine/physics/AtomStorage.h"
#include "GUI/interface/UiState.h"

RemoveAtomTool::RemoveAtomTool(ToolContext& context) noexcept : ITool(context) {}

void RemoveAtomTool::onLeftPressed(sf::Vector2i mousePos) {
    ToolContext& ctx = context();
    if (ctx.atomStorage == nullptr || ctx.atomStorage->empty() || !ctx.atomRemover || ctx.pickingSystem == nullptr) {
        return;
    }

    AtomHit hit;
    if (!ctx.pickingSystem->pickAtom(mousePos, 10.0f, hit)) {
        return;
    }

    const size_t target = hit.index;
    const auto& selected = ctx.pickingSystem->getSelectedIndices();
    const bool removeSelection = selected.contains(target);

    if (removeSelection) {
        std::vector<size_t> toRemove(selected.begin(), selected.end());
        std::sort(toRemove.begin(), toRemove.end(), std::greater<>());

        for (size_t index : toRemove) {
            if (ctx.atomRemover(index)) {
                ctx.pickingSystem->handleAtomRemoval(index);
            }
        }
    }
    else if (ctx.atomRemover(target)) {
        ctx.pickingSystem->handleAtomRemoval(target);
    }

    if (ctx.uiState != nullptr) {
        ctx.uiState->selectedAtomCount = static_cast<int>(ctx.pickingSystem->getSelectedIndices().size());
    }
}
