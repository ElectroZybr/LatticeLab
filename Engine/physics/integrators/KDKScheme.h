#pragma once

#include <vector>

class Atom;
class AtomStorage;
class ForceField;
class SimBox;

class KDKScheme {
public:
    void pipeline(std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const;
    void pipeline(AtomStorage& atomStorage, std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const;

    static void halfKick(Atom& atom, double dt);
    static void halfKick(AtomStorage& atomStorage, std::size_t atomIndex, double dt);
    static void drift(Atom& atom, double dt);
};
