#pragma once

class AtomStorage;
class ForceField;
class SimBox;

class LangevinScheme {
public:
    void pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, float dt) const;
};
