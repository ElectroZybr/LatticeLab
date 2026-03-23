#pragma once

#include <SFML/Graphics/Color.hpp>
#include <array>
#include <cstddef>
#include <vector>
#include <cstdint>

#include "../math/Vec3D.h"

class SimBox;

// Общие данные для всех атомов одного типа
struct StaticAtomicData {
    const double mass;
    const double radius;
    const char maxValence;
    const double defaultCharge;
    const sf::Color color;
    const double ljA0;
    const double ljEps;
};

class Atom {
public:
    enum class Type: uint8_t {
        Z,

        // Period 1
        H,                          He,

        // Period 2
        Li, Be,  B,  C,  N,  O,  F, Ne,

        // Period 3
        Na, Mg, Al, Si,  P,  S, Cl, Ar,

        // Period 4
        K,  Ca, Sc, Ti,  V, Cr, Mn, Fe,
        Co, Ni, Cu, Zn, Ga, Ge, As, Se, Br, Kr,

        // Period 5
        Rb, Sr,  Y, Zr, Nb, Mo, Tc, Ru,
        Rh, Pd, Ag, Cd, In, Sn, Sb, Te,  I, Xe,

        // Period 6
        Cs, Ba, La, Ce, Pr, Nd, Pm, Sm,
        Eu, Gd, Tb, Dy, Ho, Er, Tm, Yb,
        Lu, Hf, Ta,  W, Re, Os, Ir, Pt,
        Au, Hg, Tl, Pb, Bi, Po, At, Rn,

        // Period 7
        Fr, Ra, Ac, Th, Pa,  U, Np, Pu,
        Am, Cm, Bk, Cf, Es, Fm, Md, No,
        Lr, Rf, Db, Sg, Bh, Hs, Mt, Ds,
        Rg, Cn, Nh, Fl, Mc, Lv, Ts, Og,
        COUNT
    };

private:
    static const std::array<StaticAtomicData, static_cast<std::size_t>(Type::COUNT)> properties;

public:
    Type type;
    int valence;

    bool isFixed = false;
    bool isSelect = false;
    std::vector<Atom*> bonds;

    Atom (Vec3D start_coords, Vec3D start_speed, Type type, bool fixed = false);

    static float kineticEnergy(Type type, const Vec3D& speed);

    const StaticAtomicData& getProps() const {
        return properties.at(static_cast<int>(type));
    }

    static const StaticAtomicData& getProps(Atom::Type type) {
        return properties.at(static_cast<int>(type));
    }
};
