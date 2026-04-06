#include "AddAtomTool.h"

#include "Engine/SimBox.h"
#include "Engine/physics/AtomStorage.h"
#include "GUI/interface/interface.h"

AddAtomTool::AddAtomTool(ToolContext& context) noexcept : ITool(context) {}

void AddAtomTool::onLeftPressed(sf::Vector2i mousePos) {
    ToolContext& ctx = context();
    if (!ctx.isValid() || !ctx.atomCreator || ctx.box == nullptr || ctx.atomStorage == nullptr) {
        return;
    }

    const AtomData::Type atomType = static_cast<AtomData::Type>(Interface::getSelectedAtom());
    const Vec3f spawnPos = screenToWorld(mousePos);

    if (!(1 <= spawnPos.x && spawnPos.x <= ctx.box->size.x - 1 && 1 <= spawnPos.y && spawnPos.y <= ctx.box->size.y - 1 && 1 <= spawnPos.z &&
          spawnPos.z <= ctx.box->size.z - 1)) {
        return;
    }

    const float atomRadius = AtomData::getProps(atomType).radius;
    for (size_t atomIndex = 0; atomIndex < ctx.atomStorage->size(); ++atomIndex) {
        const Vec3f atomPos = ctx.atomStorage->pos(atomIndex);
        const float radius = AtomData::getProps(ctx.atomStorage->type(atomIndex)).radius;
        if ((atomPos - spawnPos).abs() <= 2.f * (radius + atomRadius)) {
            return;
        }
    }

    ctx.atomCreator(spawnPos, Vec3f::Random() * 5.f, atomType, false);
}
