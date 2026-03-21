#include "BondTable.h"

void BondTable::init() {
    // H–H (ковалентный)
    set(Atom::Type::H, Atom::Type::H, BondParams{0.7414, 100, 1.99});//0.6, 0.5, 1.0, 2.5

    // O–H (водородная связь)
    set(Atom::Type::O, Atom::Type::H, BondParams{0.9572, 100, 2.63});

    // O–O (если понадобится)
    set(Atom::Type::O, Atom::Type::O, BondParams{1.0, 200, 2.8});

    // C–C (водородная связь)
    set(Atom::Type::C, Atom::Type::C, BondParams{1.0, 300, 3.0});

    // C–H (водородная связь)
    set(Atom::Type::C, Atom::Type::H, BondParams{1.0, 100, 3.0});
}