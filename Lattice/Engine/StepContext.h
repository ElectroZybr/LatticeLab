#pragma once

class ForceField;
class IThermostat;
class NeighborList;
class World;

struct StepContext {
    World& world;
    ForceField& forceField;
    NeighborList& neighborList;
    IThermostat* thermostat = nullptr;
    bool allowBondFormation;
    bool bondsChanged = false;
    float dt;
};
