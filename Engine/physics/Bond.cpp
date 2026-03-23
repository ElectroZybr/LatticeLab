#include <algorithm>
#include <cmath>
#include <ranges>

#include "Bond.h"
#include "Atom.h"

BondTable Bond::bond_default_props;
std::list<Bond> Bond::bonds_list;

Bond::Bond(Atom* _a, Atom* _b) : a(_a), b(_b) {
    const BondParams bondParams = bond_default_props.get(_a->type, _b->type);
    params.r0 = bondParams.r0;
    params.a = bondParams.a;
    params.De = bondParams.De;
}

void Bond::forceBond(double dt) {
    (void)dt;
}

bool Bond::shouldBreak() const {
    return false;
}

float Bond::MorseForce(float distanse) {
    const float expA = std::exp(-params.a * (distanse - params.r0));
    return 2.0f * params.De * params.a * (expA * expA - expA);
}

void Bond::angleForce(Atom* o, Atom* b, Atom* c) {
    (void)o;
    (void)b;
    (void)c;
}

Bond* Bond::CreateBond(Atom* a, Atom* b) {
    bonds_list.emplace_back(a, b);
    auto it = std::prev(bonds_list.end());
    a->bonds.emplace_back(b);
    b->bonds.emplace_back(a);

    a->valence--;
    b->valence--;
    return &(*it);
}

void Bond::detach() {
    std::vector<Atom*>* bonds = &a->bonds;
    std::erase(*bonds, b);
    bonds = &b->bonds;
    std::erase(*bonds, a);

    a->valence++;
    b->valence++;
}

void Bond::BreakBond(Bond* bond) {
    if (!bond) {
        return;
    }

    bond->detach();

    if (auto it = std::ranges::find_if(bonds_list, [bond](const Bond& currentBond) { return &currentBond == bond; });
        it != bonds_list.end()) {
        bonds_list.erase(it);
    }
}
