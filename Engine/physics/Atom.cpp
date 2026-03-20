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

float Atom::kineticEnergy() const {
    return 0.5f * getProps().mass * speed.sqrAbs();
}
