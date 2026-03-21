#include <cmath>
#include <stdexcept>

#include "Vec3D.h"

namespace {
constexpr double kEps = 1e-6;
}

Vec3D::Vec3D(const Vec3D &vec) : x(vec.x), y(vec.y), z(vec.z){}

Vec3D::Vec3D(double x, double y, double z) : x(x), y(y), z(z){}

Vec3D::Vec3D(const Vec2D &vec, double z) : x(vec.x), y(vec.y), z(z){}

Vec3D::operator sf::Vector2f() const {
    return sf::Vector2f(static_cast<float>(x), static_cast<float>(y));
}

Vec3D::operator sf::Vector3f() const {
    return sf::Vector3f(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}

Vec3D Vec3D::operator-() const {
    return Vec3D(-x, -y, -z);
}

bool Vec3D::operator==(const Vec3D &vec) const {
    return (*this - vec).sqrAbs() <= kEps;
}

bool Vec3D::operator!=(const Vec3D &vec) const {
    return !(*this == vec);
}

// Operations with Vec3D
Vec3D Vec3D::operator+(const Vec3D &vec) const {
    return Vec3D(x + vec.x, y + vec.y, z + vec.z);
}

Vec3D Vec3D::operator-(const Vec3D &vec) const {
    return Vec3D(x - vec.x, y - vec.y, z - vec.z);
}

void Vec3D::operator+=(const Vec3D &vec) {
    x = x + vec.x;
    y = y + vec.y;
    z = z + vec.z;
}

void Vec3D::operator-=(const Vec3D &vec) {
    x = x - vec.x;
    y = y - vec.y;
    z = z - vec.z;
}

Vec3D Vec3D::operator-(double num) const {
    return Vec3D(x - num, y - num, z - num);
}

Vec3D Vec3D::operator*(double number) const {
    return Vec3D(x * number, y * number, z * number);
}

Vec3D Vec3D::operator*(const Vec3D &vec) const {
    return Vec3D(x * vec.x, y * vec.y, z * vec.z);
}

Vec3D Vec3D::operator/(double number) const {
    if (std::abs(number) > kEps) {
        return Vec3D(x / number, y / number, z / number);;
    }
    throw std::domain_error("Vec3D::operator/(double number): division by zero");
}

// Other useful methods
double Vec3D::sqrAbs() const {
    return x * x + y * y + z * z;
}

double Vec3D::abs() const {
    return std::sqrt(sqrAbs());
}

Vec3D Vec3D::normalized() const {
    double vecAbs = abs();
    if (vecAbs > kEps){
        return Vec3D(*this) / vecAbs;
    }
    return Vec3D(0);
}

double Vec3D::dot(const Vec3D &vec) const {
    return x * vec.x + y * vec.y + z * vec.z;
}

Vec3D Vec3D::cross(const Vec3D &vec) const {
    return Vec3D(y*vec.z-vec.y*z, 
                 z*vec.x-vec.z*x,
                 x*vec.y-vec.x*y);
}

Vec3D Vec3D::Random() {
    const double u = static_cast<double>(std::rand()) / RAND_MAX;
    const double v = static_cast<double>(std::rand()) / RAND_MAX;

    const double phi = 2.0 * std::numbers::pi * u;
    const double cos_theta = 2.0 * v - 1.0;
    const double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

    return Vec3D(
        sin_theta * std::cos(phi),
        sin_theta * std::sin(phi),
        cos_theta
    );
}

bool Vec3D::isNear(double a, double b) {
    return std::abs(a - b) < kEps;
}

double Vec3D::length() const {
    return std::sqrt(dot(*this));
}
