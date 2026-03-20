#include "Atom.h"
#include <cmath>
#include <algorithm>
#include "../SimBox.h"

SpatialGrid* Atom::grid = nullptr;

const std::array<StaticAtomicData, 118> Atom::properties = {{
        {0.0000, 0.0,  0, 0.0, sf::Color::Transparent       },
        {1.0080, 0.5,  1, 0.0, sf::Color(255, 255, 255, 255)},  // Водород
        {4.0026, 0.31, 0, 0.0, sf::Color(0,   0,   0,   255)},  // Гелий
        {6.9390, 1.67, 1, 0.0, sf::Color(0,   0,   0,   255)},  // Литий
        {9.0122, 1.12, 2, 0.0, sf::Color(0,   0,   0,   255)},  // Бериллий
        {10.811, 0.5,  3, 0.0, sf::Color(0,   0,   0,   255)},  // Бор
        {12.011, 0.5,  4, 0.0, sf::Color(0,   0,   0,   255)},  // Углерод
        {14.007, 0.5,  5, 0.0, sf::Color(80,  70,  230, 255)},  // Азот
        {15.999, 0.5,  2, 0.0, sf::Color(255, 50,  50,  255)},  // Кислород
        {18.998, 0.42, 1, 0.0, sf::Color(30,  255, 0,   255)},  // Фтор
        {20.179, 0.38, 0, 0.0, sf::Color(0,   0,   0,   255)},  // Неон
        {22.990, 1.90, 1, 0.0, sf::Color(0,   0,   0,   255)},  // Натрий
        {24.305, 1.45, 2, 0.0, sf::Color(0,   0,   0,   255)},  // Марганец
        {26.981, 1.18, 3, 0.0, sf::Color(0,   0,   0,   255)},  // Алюминий
        {28.086, 1.11, 4, 0.0, sf::Color(0,   0,   0,   255)},  // Кремний
        {30.974, 0.98, 5, 0.0, sf::Color(255, 150, 0,   255)},  // Фосфор
        {32.064, 0.88, 6, 0.0, sf::Color(255, 255, 0,   255)},  // Сера
        {35.453, 0.79, 7, 0.0, sf::Color(0,   255, 144, 255)},  // Хлор
        {39.948, 0.71, 0, 0.0, sf::Color(0,   0,   0,   255)},  // Аргон
    }};

void Atom::setGrid(SpatialGrid* grid_ptr) {
    grid = grid_ptr;
}

Atom::Atom(Vec3D start_coords, Vec3D start_speed, int type, bool fixed) : coords(start_coords), speed(start_speed), type(type), isFixed(fixed), force(0, 0), prev_force(0, 0) {
    valence = getProps().maxValence;
    bonds.reserve(getProps().maxValence);
    Bond::bond_default_props.init();
    int curr_x = grid->worldToCellX(coords.x);
    int curr_y = grid->worldToCellY(coords.y);
    int curr_z = grid->worldToCellZ(coords.z);
    grid->insert(curr_x, curr_y, curr_z, this);
}

void Atom::SoftWalls(SimBox& box, double dt) {
    const Vec3D max = box.end - box.start - Vec3D(1.0, 1.0, 1.0);
    applyWall(coords.x, speed.x, force.x, 0.0, max.x);
    applyWall(coords.y, speed.y, force.y, 0.0, max.y);
    applyWall(coords.z, speed.z, force.z, 0.0, max.z);
}

inline void Atom::applyWall(double& coord, double& speed, double& force, double min, double max) {
    constexpr double k = 500.0;
    constexpr double border = 2.0;

    // Outside the box: clamp and damp velocity only.
    // Do not additionally apply spring force from deep penetration.
    if (coord < min) {
        coord = min;
        if (speed < 0.0) speed = -speed * 0.8;
        return;
    }

    if (coord > max) {
        coord = max;
        if (speed > 0.0) speed = -speed * 0.8;
        return;
    }

    // Inside the box near the wall: soft repulsion away from the wall.
    if (coord < min + border) {
        double penetration = (min + border) - coord; // >0 near left wall
        const float p2 = penetration * penetration;   // penetration^2
        const float p4 = p2 * p2;                     // penetration^4
        force += k * p4 * p2;                         // penetration^6
    } else if (coord > max - border) {
        double penetration = coord - (max - border); // >0 near right wall
        const float p2 = penetration * penetration;   // penetration^2
        const float p4 = p2 * p2;                     // penetration^4
        force -= k * p4 * p2;                         // penetration^6
    }
}

void Atom::ComputeForces(SimBox& box, double deltaTime) {
    SoftWalls(box, deltaTime);
    int curr_x = grid->worldToCellX(coords.x);
    int curr_y = grid->worldToCellY(coords.y);
    int curr_z = grid->worldToCellZ(coords.z);
    static int range = 1;
    // проверка взаимодействий с соседними атомами
    for (int i = -range; i <= range; ++i) {
        for (int j = -range; j <= range; ++j) {
            for (int k = -range; k <= range; ++k) {
                if (auto cell = grid->at(curr_x - i, curr_y - j, curr_z - k)) {
                    for (Atom* other : *cell) {
                        // хитрожопая проверка
                        if (other <= this) continue;

                        // Vec3D delta = coords - other->coords;
                        // float distance = sqrt(delta.dot(delta));
                        
                        bool flag = std::ranges::find(bonds, other) != bonds.end();

                        if (getProps().maxValence - valence >= 2) {
                            Bond::angleForce(this, bonds[0], bonds[1]);
                        }
                        
                        if (!flag) {
                            // if (distance < 1.3 * r0 && valence > 0 && other->valence > 0) {
                            //     Bond::CreateBond(this, other);
                            // }
                            pairNonBondedInteraction(this, other);
                        }
                    }
                }
            }
        }
    }
}

void Atom::pairNonBondedInteraction(Atom *a, Atom *b) {
    /* расчет парных взаимодействий */
    Vec3D delta = b->coords - a->coords;
    float d = delta.length(); // расстояние между атомами
    Vec3D hat = delta / d;    // единичный вектор от a к b

    const float inv_d2  = 1.f / (d * d);            // 1/d^2
    const float inv_d6  = inv_d2 * inv_d2 * inv_d2; // 1/d^6
    const float a2      = a0 * a0;                  // a^2
    const float a6      = a2 * a2 * a2;             // a^6
    const float ratio6  = a6 * inv_d6;              // (a/d)^6
    const float ratio12 = ratio6 * ratio6;          // (a/d)^12

    /* силы леннард джонса */
    Vec3D force = hat * 24.0f * eps * (2.0f * ratio12 - ratio6) / d;
    a->force -= force;
    b->force += force;

    /* потенциал леннард джонса */
    // TODO: убрать расчет на каждой итерации
    float potential = 4.0f * eps * (ratio12 - ratio6);
    a->potential_energy += 0.5f * potential;
    b->potential_energy += 0.5f * potential;
}

float Atom::kineticEnergy() const {
    return 0.5f * getProps().mass * speed.sqrAbs();
}
