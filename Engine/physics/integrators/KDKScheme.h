#pragma once

#include <cstddef>

class AtomStorage;
class ForceField;
class SimBox;

class KDKScheme {
public:
    void pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, float dt) const;

    static void halfKick(AtomStorage& atomStorage, std::size_t atomIndex, float dt);
    static void drift(AtomStorage& atomStorage, std::size_t atomIndex, float dt);
};
