#include "WallForceField.h"

void WallForceField::syncWalls(const SimBox& box) {
    wallMaxX_ = box.size.x - 1.0f;
    wallMaxY_ = box.size.y - 1.0f;
    wallMaxZ_ = box.size.z - 1.0f;
}

void WallForceField::compute(AtomStorage& atoms, const Vec3f& gravity) const {
    for (size_t atomIndex = 0; atomIndex < atoms.mobileCount(); ++atomIndex) {
        float forceX = atoms.forceX(atomIndex);
        float forceY = atoms.forceY(atomIndex);
        float forceZ = atoms.forceZ(atomIndex);

        // мягкие стены
        softWalls(atoms.posX(atomIndex), atoms.posY(atomIndex), atoms.posZ(atomIndex), forceX, forceY, forceZ);
        // постоянная сила
        applyGravityForce(forceX, forceY, forceZ, gravity);

        atoms.forceX(atomIndex) = forceX;
        atoms.forceY(atomIndex) = forceY;
        atoms.forceZ(atomIndex) = forceZ;
    }
}

void WallForceField::applyWall(float coord, float& force, float max) {
    constexpr float k = 500.0f;
    constexpr float border = 2.0f;

    if (coord <= 0.0f || coord >= max) {
        return;
    }

    if (coord < border) {
        const float penetration = border - coord;
        const float p2 = penetration * penetration;
        const float p4 = p2 * p2;
        force += k * p4 * p2;
    } else if (coord > max - border) {
        const float penetration = coord - (max - border);
        const float p2 = penetration * penetration;
        const float p4 = p2 * p2;
        force -= k * p4 * p2;
    }
}

void WallForceField::softWalls(float coordX, float coordY, float coordZ, float& forceX, float& forceY, float& forceZ) const {
    applyWall(coordX, forceX, wallMaxX_);
    applyWall(coordY, forceY, wallMaxY_);
    applyWall(coordZ, forceZ, wallMaxZ_);
}

void WallForceField::applyGravityForce(float& forceX, float& forceY, float& forceZ, const Vec3f& gravity) {
    forceX += gravity.x;
    forceY += gravity.y;
    forceZ += gravity.z;
}
