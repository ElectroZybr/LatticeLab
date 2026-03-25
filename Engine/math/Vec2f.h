#pragma once
#include <cmath>
#include <numbers>
#include <stdexcept>

#include <SFML/System/Vector2.hpp>

#include "Engine/math/Consts.h"

namespace {
}

class Vec2f {
public:
    float x, y;

    Vec2f(const Vec2f& vec) : x(vec.x), y(vec.y) {}
    explicit Vec2f(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

    operator sf::Vector2f() const { return sf::Vector2f(static_cast<float>(x), static_cast<float>(y)); }

    [[nodiscard]] Vec2f operator-() const { return Vec2f(-x, -y); }

    bool operator==(const Vec2f& vec) const { return (*this - vec).sqrAbs() < Consts::Epsilon; }
    bool operator!=(const Vec2f& vec) const { return !(*this == vec); }

    [[nodiscard]] Vec2f operator+(const Vec2f& vec) const { return Vec2f(x + vec.x, y + vec.y); }
    [[nodiscard]] Vec2f operator-(const Vec2f& vec) const { return Vec2f(x - vec.x, y - vec.y); }
    [[nodiscard]] Vec2f operator-(float num) const { return Vec2f(x - num, y - num); }

    void operator+=(const Vec2f& vec) { x += vec.x; y += vec.y; }
    void operator-=(const Vec2f& vec) { x -= vec.x; y -= vec.y; }

    [[nodiscard]] Vec2f operator*(float number) const { return Vec2f(x * number, y * number); }
    [[nodiscard]] Vec2f operator/(float number) const {
        if (std::abs(number) > Consts::Epsilon)
            return Vec2f(x / number, y / number);
        throw std::domain_error("Vec2f::operator/: division by zero");
    }

    [[nodiscard]] float sqrAbs()               const { return x * x + y * y; }
    [[nodiscard]] float abs()                  const { return std::sqrt(sqrAbs()); }
    [[nodiscard]] float dot(const Vec2f& vec) const { return x * vec.x + y * vec.y; }

    [[nodiscard]] Vec2f normalized() const {
        float vecAbs = abs();
        return vecAbs > Consts::Epsilon ? Vec2f(*this) / vecAbs : Vec2f(0);
    }

    static Vec2f Random() {
        const float phi = 2.0f * std::numbers::pi_v<float> * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
        return Vec2f(std::cos(phi), std::sin(phi));
    }

private:
    static bool isNear(float a, float b) { return std::abs(a - b) < Consts::Epsilon; }
};
