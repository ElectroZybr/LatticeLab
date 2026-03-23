#pragma once

#include <cstddef>

class AtomStorage;
class ForceField;
class SimBox;

class VerletScheme {
public:
    void pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, float dt) const;

    static void predict(AtomStorage& atomStorage, std::size_t atomIndex, float dt);
    static void correct(AtomStorage& atomStorage, std::size_t atomIndex, float dt);
};
