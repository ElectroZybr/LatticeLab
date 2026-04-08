#include "AddAtomTool.h"

#include "Engine/Simulation.h"
#include "Engine/physics/AtomStorage.h"
#include "GUI/interface/UiState.h"
#include "GUI/interface/panels/periodic/PeriodicPanel.h"

AddAtomTool::AddAtomTool(ToolContext& context) noexcept : ITool(context) {}

void AddAtomTool::onLeftPressed(sf::Vector2i mousePos) {
    ToolContext& ctx = context();
    if (!ctx.isValid()) {
        return;
    }

    if (ctx.uiState == nullptr) {
        return;
    }

    AtomStorage& atoms = ctx.simulation->atoms();
    const SimBox& box = ctx.simulation->box();
    const AtomData::Type atomType = static_cast<AtomData::Type>(PeriodicPanel::decodeAtom(ctx.uiState->selectedAtom));
    const Vec3f spawnPos = screenToWorld(mousePos);

    if (!(1 <= spawnPos.x && spawnPos.x <= box.size.x - 1 && 1 <= spawnPos.y && spawnPos.y <= box.size.y - 1 && 1 <= spawnPos.z &&
          spawnPos.z <= box.size.z - 1)) {
        return;
    }

    const float atomRadius = AtomData::getProps(atomType).radius;
    for (size_t atomIndex = 0; atomIndex < atoms.size(); ++atomIndex) {
        const Vec3f atomPos = atoms.pos(atomIndex);
        const float radius = AtomData::getProps(atoms.type(atomIndex)).radius;
        if ((atomPos - spawnPos).abs() <= 2.f * (radius + atomRadius)) {
            return;
        }
    }

    ctx.simulation->createAtom(spawnPos, Vec3f::Random() * 5.f, atomType, false);
}
