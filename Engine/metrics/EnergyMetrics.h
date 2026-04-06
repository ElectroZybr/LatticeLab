#pragma once

#include "Engine/physics/AtomData.h"
#include "Engine/physics/AtomStorage.h"

namespace EnergyMetrics {

    inline float kineticEnergy(AtomData::Type type, const Vec3f& speed) { return 0.5f * AtomData::getProps(type).mass * speed.sqrAbs(); }

    inline float averageKineticEnergy(const AtomStorage& atomStorage) {
        if (atomStorage.empty()) {
            return 0.0f;
        }

        double totalEnergy = 0.0;
        for (size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
            totalEnergy += kineticEnergy(atomStorage.type(atomIndex), atomStorage.vel(atomIndex));
        }

        return static_cast<float>(totalEnergy / static_cast<double>(atomStorage.size()));
    }

    inline float averagePotentialEnergy(const AtomStorage& atomStorage) {
        if (atomStorage.empty()) {
            return 0.0f;
        }

        double totalEnergy = 0.0;
        for (size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
            totalEnergy += atomStorage.energy(atomIndex);
        }

        return static_cast<float>(totalEnergy / static_cast<double>(atomStorage.size()));
    }

    inline float fullAverageEnergy(const AtomStorage& atomStorage) {
        return averageKineticEnergy(atomStorage) + averagePotentialEnergy(atomStorage);
    }

} // namespace EnergyMetrics
