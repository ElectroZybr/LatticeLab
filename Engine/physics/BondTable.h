#pragma once

#include "Atom.h"

struct BondParams {
    float r0=0;
    float De=0;
    float a=0;
};

struct BondTable {
    // Матрица параметров "тип1 -> тип2"
    BondParams table[(int)Atom::Type::COUNT][(int)Atom::Type::COUNT];

    void init();
    // инициализация параметров для пары
    void set(Atom::Type a, Atom::Type b, const BondParams& p) {
        table[(int)a][(int)b] = p;
        table[(int)b][(int)a] = p; // симметрия
    }

    // получить параметры
    const BondParams& get(Atom::Type a, Atom::Type b) const {
        return table[(int)a][(int)b];
    }
};

struct AngleTable {
    // Матрица параметров "тип1 -> тип2"
    BondParams table[(int)Atom::Type::COUNT][(int)Atom::Type::COUNT];

    void init();
    // инициализация параметров для пары
    void set(Atom::Type a, Atom::Type b, const BondParams& p) {
        table[(int)a][(int)b] = p;
        table[(int)b][(int)a] = p; // симметрия
    }

    // получить параметры
    const BondParams& get(Atom::Type a, Atom::Type b) const {
        return table[(int)a][(int)b];
    }
};