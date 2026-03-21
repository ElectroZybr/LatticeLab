#include <cmath>

#include "Bond.h"
#include "Atom.h"
#include <numbers>

BondTable Bond::bond_default_props;
std::list<Bond> Bond::bonds_list;


Bond::Bond (Atom* _a, Atom* _b) : a(_a), b(_b) {//, float _r0, float _k, float _D_e, float _alpha
    BondParams bond_params = bond_default_props.get(_a->type, _b->type);
    // std::cout << "<Bond params> a:" << _a->type << " b: "<< _b->type << " r0: "<< bond_params.r0 << " a: "<< bond_params.a << " De: "<< bond_params.De << std::endl;
    params.r0 = bond_params.r0;
    params.a = bond_params.a;
    params.De = bond_params.De;
}

void Bond::forceBond(double dt) {
    Vec3D delta = a->coords - b->coords;
    float len = delta.abs();
    Vec3D hat = delta / len;
    Vec3D force = hat * MorseForce(len);

    a->force += force;
    b->force -= force;
}

bool Bond::shouldBreak() const {
    Vec3D delta = a->coords - b->coords;
    float distanseSqr = delta.dot(delta);
    return distanseSqr > 3.f*3.f;
}

float Bond::MorseForce(float distanse) {
    /* производная потенциала Морзе по расстоянию */
    float exp_a = std::exp(-params.a * (distanse - params.r0));
    return 2 * params.De * params.a * (exp_a * exp_a - exp_a);
}

void Bond::angleForce(Atom* o, Atom* b, Atom* c) {
    /* Атом o - центральный, b и c - присоединенные */
    Vec3D delta_ob = b->coords - o->coords; // Вектор направления ob
    Vec3D delta_oc = c->coords - o->coords; // Вектор направления oc

    double len_ob = delta_ob.abs(); // Скаляр вектора ob
    double len_oc = delta_oc.abs(); // Скаляр вектора oc

    Vec3D ob_hat = delta_ob / len_ob; // нормализованный вектор направления ob
    Vec3D oc_hat = delta_oc / len_oc; // нормализованный вектор направления oc

    double cos_theta = ob_hat.dot(oc_hat); // косинус угла theta
    double sin_theta_sqr = 1.0-cos_theta*cos_theta; // квадрат синуса угла theta
    if (sin_theta_sqr < 1e-12) return;

    double angle_theta = std::acos(cos_theta); // Угол theta в радианах
    constexpr double theta_0 = 60.0 / 180.0 * std::numbers::pi; // Заданный угол theta в градусах
    double angle_loss = angle_theta - theta_0; // Текущая ошибка угла
    
    double sin_theta = std::sqrt(sin_theta_sqr);
    
    constexpr double k = 50;
    Vec3D force_b = -((oc_hat - ob_hat * cos_theta) / len_ob) * (-k * angle_loss / sin_theta); // градиент сил b
    Vec3D force_c = -((ob_hat - oc_hat * cos_theta) / len_oc) * (-k * angle_loss / sin_theta); // градиент сил c
    Vec3D force_o = -(force_b + force_c);

    b->force += force_b;
    c->force += force_c;
    o->force += force_o;
}

Bond* Bond::CreateBond(Atom* a, Atom* b) {
    // std::cout << "<Create bond>" << std::endl;
    bonds_list.emplace_back(a, b);
    auto it = std::prev(bonds_list.end());
    a->bonds.emplace_back(b);
    b->bonds.emplace_back(a);

    a->valence--;
    b->valence--;
    return &(*it);
}

void Bond::detach() {
    std::vector<Atom*>* bonds = &a->bonds;
    std::erase(*bonds, b);
    bonds = &b->bonds;
    std::erase(*bonds, a);

    a->valence++;
    b->valence++;
}

void Bond::BreakBond(Bond* bond) {
    if (!bond) return;
    // std::cout << "<Break bond>" << std::endl;
    bond->detach();

    if (auto it = std::ranges::find_if(bonds_list, [bond](const Bond& b) { return &b == bond; });
        it != bonds_list.end()) {
        bonds_list.erase(it);
    }
}
