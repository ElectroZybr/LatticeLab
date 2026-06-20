#include "BondTable.h"

void BondTable::init() {
    // H–H (ковалентный)
    set(AtomData::Type::H, AtomData::Type::H, BondParams{0.7414, 100, 1.99}); // 0.6, 0.5, 1.0, 2.5

    // H–F (HF)
    set(AtomData::Type::H, AtomData::Type::F, BondParams{0.917f, 130.0f, 2.9f});

    // H–Cl (HCl)
    set(AtomData::Type::H, AtomData::Type::Cl, BondParams{1.275f, 120.0f, 2.4f});

    // N–N (N2)
    set(AtomData::Type::N, AtomData::Type::N, BondParams{1.10f, 350.0f, 3.4f});

    // N–H (NH3)
    set(AtomData::Type::N, AtomData::Type::H, BondParams{1.014f, 120.0f, 2.8f});

    // N–O (NO)
    set(AtomData::Type::N, AtomData::Type::O, BondParams{1.150f, 220.0f, 3.0f});

    // O–H (водородная связь)
    set(AtomData::Type::O, AtomData::Type::H, BondParams{0.9572, 100, 2.63});

    // O–O (если понадобится)
    set(AtomData::Type::O, AtomData::Type::O, BondParams{1.0, 200, 2.8});

    // C–O (CO2)
    set(AtomData::Type::C, AtomData::Type::O, BondParams{1.160f, 260.0f, 3.1f});

    // C–C (водородная связь)
    set(AtomData::Type::C, AtomData::Type::C, BondParams{1.0, 300, 3.0});

    // C–H (водородная связь)
    set(AtomData::Type::C, AtomData::Type::H, BondParams{1.0, 100, 3.0});

    // F–F (F2)
    set(AtomData::Type::F, AtomData::Type::F, BondParams{1.420f, 180.0f, 2.8f});

    // Cl–Cl (Cl2)
    set(AtomData::Type::Cl, AtomData::Type::Cl, BondParams{1.990f, 170.0f, 2.2f});

    // Br–Br (Br2)
    set(AtomData::Type::Br, AtomData::Type::Br, BondParams{2.280f, 160.0f, 2.0f});
}
