#include <cmath>
#include <numbers>
#include <stdexcept>

#include "Vec2D.h"
#include "Vec3D.h"

namespace {
constexpr double kEps = 1e-6;
}

Vec2D::Vec2D(const Vec2D &vec) : x(vec.x), y(vec.y){}

Vec2D::Vec2D(double x, double y) : x(x), y(y) {}

Vec2D::Vec2D(const Vec3D& vec3d) : x(vec3d.x), y(vec3d.y) {}


Vec2D::operator sf::Vector2f() const {
    return sf::Vector2f(static_cast<float>(x), static_cast<float>(y));
}

Vec2D Vec2D::operator-() const {
    return Vec2D(-x, -y);
}

bool Vec2D::operator==(const Vec2D &vec) const {
    Vec2D diff = *this - vec;
    return diff.sqrAbs() < kEps;
}

bool Vec2D::operator!=(const Vec2D &vec) const {
    return !(*this == vec);
}

Vec2D Vec2D::operator+(const Vec2D &vec) const {
    return Vec2D(x + vec.x, y + vec.y);
}

Vec2D Vec2D::operator-(const Vec2D &vec) const {
    return Vec2D(x - vec.x, y - vec.y);
}

Vec2D Vec2D::operator-(double num) const {
    return Vec2D(x - num, y - num);
}

void Vec2D::operator+=(const Vec2D &vec) {
    x = x + vec.x;
    y = y + vec.y;
}

void Vec2D::operator-=(const Vec2D &vec) {
    x = x - vec.x;
    y = y - vec.y;
}

Vec2D Vec2D::operator*(double number) const {
    return Vec2D(x * number, y * number);
}

Vec2D Vec2D::operator/(double number) const {
    if (std::abs(number) > kEps){
        return Vec2D(x / number, y / number);
    }
    throw std::domain_error("Vec2D::operator/(double number): division by zero");
}

// Other useful methods
double Vec2D::sqrAbs() const {
    return x * x + y * y;
}

double Vec2D::abs() const {
    return std::sqrt(sqrAbs());
}

Vec2D Vec2D::normalized() const {
    double vecAbs = abs();
    if (vecAbs > kEps){
        return Vec2D(*this) / vecAbs;
    }
    return Vec2D(0);
}

double Vec2D::dot(const Vec2D &vec) const {
    return x * vec.x + y * vec.y;
}

bool Vec2D::isNear(double a, double b) {
    return std::abs(a - b) < kEps;
}

double Vec2D::length() const {
    return sqrt(x*x + y*y);
}


Vec2D Vec2D::Random() {
    const double u = static_cast<double>(std::rand()) / RAND_MAX;
    const double phi = 2.0 * std::numbers::pi * u;

    return Vec2D(
        std::cos(phi),
        std::sin(phi)
    );
}