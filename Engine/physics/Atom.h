#pragma once

#include <SFML/Graphics/Color.hpp>
#include <array>
#include "../math/Vec2D.h"
#include "../math/Vec3D.h"
#include "Bond.h"
#include <vector>

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
private:
    static const std::array<StaticAtomicData, 118> properties;
public:
    Vec3D coords;
    Vec3D speed;
    Vec3D force;
    Vec3D prev_force;

    int type;
    int valence;
    float potential_energy = 0.0;

    bool isFixed = false;
    bool isSelect = false;
    std::vector<Atom*> bonds;

    Atom (Vec3D start_coords, Vec3D start_speed, int type, bool fixed = false);

    float kineticEnergy() const;

    const StaticAtomicData& getProps() const {
        return properties.at(type);
    }

    static const StaticAtomicData& getProps(int type) {
        return properties.at(type);
    }
};
