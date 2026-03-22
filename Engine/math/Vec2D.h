#pragma once
#include <cmath>
#include <numbers>
#include <stdexcept>

#include <SFML/System/Vector2.hpp>

#include "Engine/math/Consts.h"

namespace {
}

class Vec2D {
public:
    double x, y;

    Vec2D(const Vec2D &vec) : x(vec.x), y(vec.y) {}
    explicit Vec2D(double x = 0.0, double y = 0.0) : x(x), y(y) {}

    operator sf::Vector2f() const { return sf::Vector2f(static_cast<float>(x), static_cast<float>(y)); }

    [[nodiscard]] Vec2D operator-() const { return Vec2D(-x, -y); }

    bool operator==(const Vec2D &vec) const { return (*this - vec).sqrAbs() < Consts::Epsilon; }
    bool operator!=(const Vec2D &vec) const { return !(*this == vec); }

    [[nodiscard]] Vec2D operator+(const Vec2D &vec) const { return Vec2D(x + vec.x, y + vec.y); }
    [[nodiscard]] Vec2D operator-(const Vec2D &vec) const { return Vec2D(x - vec.x, y - vec.y); }
    [[nodiscard]] Vec2D operator-(double num)        const { return Vec2D(x - num,   y - num); }

    void operator+=(const Vec2D &vec) { x += vec.x; y += vec.y; }
    void operator-=(const Vec2D &vec) { x -= vec.x; y -= vec.y; }

    [[nodiscard]] Vec2D operator*(double number) const { return Vec2D(x * number, y * number); }
    [[nodiscard]] Vec2D operator/(double number) const {
        if (std::abs(number) > Consts::Epsilon)
            return Vec2D(x / number, y / number);
        throw std::domain_error("Vec2D::operator/: division by zero");
    }

    [[nodiscard]] double sqrAbs()                const { return x * x + y * y; }
    [[nodiscard]] double abs()                   const { return std::sqrt(sqrAbs()); }
    [[nodiscard]] double dot(const Vec2D &vec)   const { return x * vec.x + y * vec.y; }

    [[nodiscard]] Vec2D normalized() const {
        double vecAbs = abs();
        return vecAbs > Consts::Epsilon ? Vec2D(*this) / vecAbs : Vec2D(0);
    }

    static Vec2D Random() {
        const double phi = 2.0 * std::numbers::pi * (static_cast<double>(std::rand()) / RAND_MAX);
        return Vec2D(std::cos(phi), std::sin(phi));
    }

private:
    static bool isNear(double a, double b) { return std::abs(a - b) < Consts::Epsilon; }
};