#include <algorithm>
#include <cmath>
#include <ranges>

#include "Bond.h"
#include "AtomData.h"
#include "AtomStorage.h"

BondTable Bond::bond_default_props;
std::list<Bond> Bond::bonds_list;

Bond::Bond(std::size_t aIndex, std::size_t bIndex, AtomData::Type aType, AtomData::Type bType)
    : aIndex(aIndex), bIndex(bIndex) {
    const BondParams bondParams = bond_default_props.get(aType, bType);
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

void Bond::angleForce(std::size_t aIndex, std::size_t bIndex, std::size_t cIndex) {
    (void)aIndex;
    (void)bIndex;
    (void)cIndex;
}

Bond* Bond::CreateBond(std::size_t aIndex, std::size_t bIndex, const AtomStorage& atomStorage) {
    if (aIndex >= atomStorage.size() || bIndex >= atomStorage.size()) {
        return nullptr;
    }

    bonds_list.emplace_back(aIndex, bIndex, atomStorage.type(aIndex), atomStorage.type(bIndex));
    return &bonds_list.back();
}

void Bond::detach() {
}

void Bond::BreakBond(Bond* bond) {
    if (!bond) {
        return;
    }

    if (auto it = std::ranges::find_if(bonds_list, [bond](const Bond& currentBond) { return &currentBond == bond; });
        it != bonds_list.end()) {
        bonds_list.erase(it);
    }
}

