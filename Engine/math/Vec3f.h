#pragma once

#include <cmath>
#include <numbers>
#include <stdexcept>

#include <SFML/System/Vector3.hpp>

#include "Engine/math/Consts.h"
#include "Vec2f.h"

class alignas(32) Vec3f final {
public:
    float x, y, z;

    Vec3f(const Vec3f& vec) : x(vec.x), y(vec.y), z(vec.z) {}
    explicit Vec3f(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}
    Vec3f(const Vec2f& vec, float z = 0.0f) : x(vec.x), y(vec.y), z(z) {}

    operator sf::Vector2f() const { return sf::Vector2f(static_cast<float>(x), static_cast<float>(y)); }
    operator sf::Vector3f() const { return sf::Vector3f(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)); }

    [[nodiscard]] Vec3f operator-() const { return Vec3f(-x, -y, -z); }

    bool operator==(const Vec3f& vec) const { return (*this - vec).sqrAbs() <= Consts::Epsilon; }
    bool operator!=(const Vec3f& vec) const { return !(*this == vec); }

    [[nodiscard]] Vec3f operator+(const Vec3f& vec) const { return Vec3f(x + vec.x, y + vec.y, z + vec.z); }
    [[nodiscard]] Vec3f operator-(const Vec3f& vec) const { return Vec3f(x - vec.x, y - vec.y, z - vec.z); }
    [[nodiscard]] Vec3f operator+(float num) const { return Vec3f(x + num, y + num, z + num); }
    [[nodiscard]] Vec3f operator-(float num) const { return Vec3f(x - num, y - num, z - num); }

    void operator+=(const Vec3f& vec) { x += vec.x; y += vec.y; z += vec.z; }
    void operator-=(const Vec3f& vec) { x -= vec.x; y -= vec.y; z -= vec.z; }

    [[nodiscard]] Vec3f operator*(float number) const { return Vec3f(x * number, y * number, z * number); }
    [[nodiscard]] Vec3f operator*(const Vec3f& vec) const { return Vec3f(x * vec.x, y * vec.y, z * vec.z); }
    [[nodiscard]] Vec3f operator/(float number) const {
        if (std::abs(number) > Consts::Epsilon)
            return Vec3f(x / number, y / number, z / number);
        throw std::domain_error("Vec3f::operator/: division by zero");
    }

    [[nodiscard]] float sqrAbs()                    const { return x*x + y*y + z*z; }
    [[nodiscard]] float abs()                       const { return std::sqrt(sqrAbs()); }
    [[nodiscard]] float dot(const Vec3f& vec) const { return x * vec.x + y * vec.y + z * vec.z; }

    [[nodiscard]] Vec3f cross(const Vec3f& vec) const {
        return Vec3f(
            y*vec.z - vec.y*z,
            z*vec.x - vec.z*x,
            x*vec.y - vec.x*y
        );
    }

    [[nodiscard]] Vec3f normalized() const {
        float vecAbs = abs();
        return vecAbs > Consts::Epsilon ? Vec3f(*this) / vecAbs : Vec3f(0);
    }

    static Vec3f Random() {
        const float phi       = 2.0f * std::numbers::pi_v<float> * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
        const float cos_theta = 2.0f * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) - 1.0f;
        const float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);
        return Vec3f(sin_theta * std::cos(phi), sin_theta * std::sin(phi), cos_theta);
    }
};
