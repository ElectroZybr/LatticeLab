#pragma once

#include <SFML/Graphics/Color.hpp>
#include <array>
#include <cstddef>
#include <cstdint>

#include "../math/Vec3f.h"

struct StaticAtomicData {
    const float mass;
    const float radius;
    const char maxValence;
    const float defaultCharge;
    const sf::Color color;
    const float ljA0;
    const float ljEps;
};

class AtomData {
public:
    enum class Type : std::uint8_t {
        Z,

        H, He,

        Li, Be, B, C, N, O, F, Ne,

        Na, Mg, Al, Si, P, S, Cl, Ar,

        K, Ca, Sc, Ti, V, Cr, Mn, Fe,
        Co, Ni, Cu, Zn, Ga, Ge, As, Se, Br, Kr,

        Rb, Sr, Y, Zr, Nb, Mo, Tc, Ru,
        Rh, Pd, Ag, Cd, In, Sn, Sb, Te, I, Xe,

        Cs, Ba, La, Ce, Pr, Nd, Pm, Sm,
        Eu, Gd, Tb, Dy, Ho, Er, Tm, Yb,
        Lu, Hf, Ta, W, Re, Os, Ir, Pt,
        Au, Hg, Tl, Pb, Bi, Po, At, Rn,

        Fr, Ra, Ac, Th, Pa, U, Np, Pu,
        Am, Cm, Bk, Cf, Es, Fm, Md, No,
        Lr, Rf, Db, Sg, Bh, Hs, Mt, Ds,
        Rg, Cn, Nh, Fl, Mc, Lv, Ts, Og,
        COUNT
    };

private:
    static const std::array<StaticAtomicData, static_cast<std::size_t>(Type::COUNT)> properties;

public:
    Type type{};
    bool isFixed = false;
    bool isSelect = false;

    AtomData(Vec3f startCoords, Vec3f startSpeed, Type type, bool fixed = false);

    const StaticAtomicData& getProps() const {
        return properties.at(static_cast<int>(type));
    }

    static const StaticAtomicData& getProps(Type type) {
        return properties.at(static_cast<int>(type));
    }
};