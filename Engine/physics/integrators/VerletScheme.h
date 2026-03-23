#pragma once

#include <vector>

class Atom;
class AtomStorage;
class ForceField;
class SimBox;

class VerletScheme {
public:
    void pipeline(std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const;
    void pipeline(AtomStorage& atomStorage, std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const;

    static void predict(Atom& atom, double dt);
    static void predict(AtomStorage& atomStorage, std::size_t atomIndex, double dt);
    static void correct(Atom& atom, double dt);
    static void correct(AtomStorage& atomStorage, std::size_t atomIndex, double dt);
};
