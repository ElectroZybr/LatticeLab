#pragma once

#include "BondTable.h"
#include <cstddef>
#include <list>

class AtomStorage;


class Bond {
private:
public:
    static BondTable bond_default_props;

    static Bond* CreateBond(std::size_t aIndex, std::size_t bIndex, AtomStorage& atomStorage);
    static void BreakBond(Bond* bond, AtomStorage& atomStorage);
    static std::list<Bond> bonds_list;
    static void angleForce(AtomStorage& atomStorage, std::size_t aIndex, std::size_t bIndex, std::size_t cIndex);

    Bond(std::size_t aIndex, std::size_t bIndex, AtomData::Type aType, AtomData::Type bType);

    void forceBond(AtomStorage& atomStorage, float dt);
    bool shouldBreak(const AtomStorage& atomStorage) const;
    void detach(AtomStorage& atomStorage);
    float MorseForce(float distanse);

    std::size_t aIndex;
    std::size_t bIndex;

    BondParams params;
};

