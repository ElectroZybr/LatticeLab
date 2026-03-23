#pragma once

#include "BondTable.h"
#include <cstddef>
#include <list>

class AtomStorage;


class Bond {
private:
public:
    static BondTable bond_default_props;

    static Bond* CreateBond(std::size_t aIndex, std::size_t bIndex, const AtomStorage& atomStorage);
    static void BreakBond(Bond* bond);
    static std::list<Bond> bonds_list;
    static void angleForce(std::size_t aIndex, std::size_t bIndex, std::size_t cIndex);

    Bond(std::size_t aIndex, std::size_t bIndex, AtomData::Type aType, AtomData::Type bType);

    void forceBond(double dt);
    bool shouldBreak() const;
    void detach();
    float MorseForce(float distanse);

    std::size_t aIndex;
    std::size_t bIndex;

    BondParams params;
};

