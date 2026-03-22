#pragma once

#include <cmath>
#include <numbers>
#include <stdexcept>

#include <SFML/System/Vector3.hpp>

#include "Engine/math/Consts.h"
#include "Vec2D.h"

class alignas(32) Vec3D final {
public:
    double x, y, z;

    Vec3D(const Vec3D &vec) : x(vec.x), y(vec.y), z(vec.z) {}
    explicit Vec3D(double x = 0.0, double y = 0.0, double z = 0.0) : x(x), y(y), z(z) {}
    Vec3D(const Vec2D &vec, double z = 0.0) : x(vec.x), y(vec.y), z(z) {}

    operator sf::Vector2f() const { return sf::Vector2f(static_cast<float>(x), static_cast<float>(y)); }
    operator sf::Vector3f() const { return sf::Vector3f(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)); }

    [[nodiscard]] Vec3D operator-() const { return Vec3D(-x, -y, -z); }

    bool operator==(const Vec3D &vec) const { return (*this - vec).sqrAbs() <= Consts::Epsilon; }
    bool operator!=(const Vec3D &vec) const { return !(*this == vec); }

    [[nodiscard]] Vec3D operator+(const Vec3D &vec) const { return Vec3D(x + vec.x, y + vec.y, z + vec.z); }
    [[nodiscard]] Vec3D operator-(const Vec3D &vec) const { return Vec3D(x - vec.x, y - vec.y, z - vec.z); }
    [[nodiscard]] Vec3D operator+(double num)        const { return Vec3D(x + num,   y + num,   z + num); }
    [[nodiscard]] Vec3D operator-(double num)        const { return Vec3D(x - num,   y - num,   z - num); }

    void operator+=(const Vec3D &vec) { x += vec.x; y += vec.y; z += vec.z; }
    void operator-=(const Vec3D &vec) { x -= vec.x; y -= vec.y; z -= vec.z; }

    [[nodiscard]] Vec3D operator*(double number)     const { return Vec3D(x * number,  y * number,  z * number); }
    [[nodiscard]] Vec3D operator*(const Vec3D &vec)  const { return Vec3D(x * vec.x,   y * vec.y,   z * vec.z); }
    [[nodiscard]] Vec3D operator/(double number)     const {
        if (std::abs(number) > Consts::Epsilon)
            return Vec3D(x / number, y / number, z / number);
        throw std::domain_error("Vec3D::operator/: division by zero");
    }

    [[nodiscard]] double sqrAbs()                   const { return x*x + y*y + z*z; }
    [[nodiscard]] double abs()                      const { return std::sqrt(sqrAbs()); }
    [[nodiscard]] double dot(const Vec3D &vec)      const { return x*vec.x + y*vec.y + z*vec.z; }

    [[nodiscard]] Vec3D cross(const Vec3D &vec) const {
        return Vec3D(
            y*vec.z - vec.y*z,
            z*vec.x - vec.z*x,
            x*vec.y - vec.x*y
        );
    }

    [[nodiscard]] Vec3D normalized() const {
        double vecAbs = abs();
        return vecAbs > Consts::Epsilon ? Vec3D(*this) / vecAbs : Vec3D(0);
    }

    static Vec3D Random() {
        const double phi       = 2.0 * std::numbers::pi * (static_cast<double>(std::rand()) / RAND_MAX);
        const double cos_theta = 2.0 * (static_cast<double>(std::rand()) / RAND_MAX) - 1.0;
        const double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);
        return Vec3D(sin_theta * std::cos(phi), sin_theta * std::sin(phi), cos_theta);
    }
};