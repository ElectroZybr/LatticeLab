#pragma once

#include "Lattice/Engine/physics/IForceField.h"
#include "Lattice/Plugins/ClassicMD/ForceFields/BondForceField.h"
#include "Lattice/Plugins/ClassicMD/ForceFields/CoulombForceField.h"
#include "Lattice/Plugins/ClassicMD/ForceFields/LJForceField.h"
#include "Lattice/Plugins/ClassicMD/ForceFields/WallForceField.h"

class ClassicMDForceField final : public IForceField {
public:
    static constexpr std::string_view id = "classic_md";
    static constexpr std::string_view description = "forcefield_classic_md";

    bool compute(World& world, bool allowBondFormation, float dt) const override;
    FieldSample fieldAtPoint(const AtomStorage& atoms, const SpatialGrid& grid, float x, float y, float z) const override;
    void computePairInteractions(World& world) const;

private:
    WallForceField wallForceField_{};
    LJForceField ljForceField_{};
    BondForceField bondForceField_{};
    CoulombForceField coulombForceField_{};
};
