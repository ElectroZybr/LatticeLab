#pragma once

#include "BondTable.h"
#include <cstddef>
#include <list>

class AtomStorage;

class Bond {
private:
public:
    static BondTable bond_default_props;
    using List = std::list<Bond>;

    static void ensureInitialized();
    static Bond* CreateBond(List& bonds, size_t aIndex, size_t bIndex, AtomStorage& atomStorage);
    static void BreakBond(List& bonds, Bond* bond, AtomStorage& atomStorage);
    static void angleForce(AtomStorage& atomStorage, size_t aIndex, size_t bIndex, size_t cIndex);

    Bond(size_t aIndex, size_t bIndex, AtomData::Type aType, AtomData::Type bType);

    void forceBond(AtomStorage& atomStorage, float dt);
    bool shouldBreak(const AtomStorage& atomStorage) const;
    void detach(AtomStorage& atomStorage);
    float MorseForce(float distanse);

    size_t aIndex;
    size_t bIndex;

    BondParams params;
};

