#pragma once
#include <SFML/System/Vector2.hpp>

class Vec3D;

class Vec2D {
private:
    static bool isNear(double a, double b);
    
public:
    double x, y;

    Vec2D(const Vec2D &vec);

    explicit Vec2D(double x = 0.0, double y = 0.0);
    explicit Vec2D(const Vec3D& vec3d);

    // Операторы приведения типа
    operator sf::Vector2f() const;

    [[nodiscard]] Vec2D operator-() const;

    // Boolean operations
    bool operator==(const Vec2D &vec) const;
    bool operator!=(const Vec2D &vec) const;

    [[nodiscard]] Vec2D operator+(const Vec2D &vec) const;
    [[nodiscard]] Vec2D operator-(const Vec2D &vec) const;
    [[nodiscard]] Vec2D operator-(double num) const;
    void operator+=(const Vec2D &vec);
    void operator-=(const Vec2D &vec);

    [[nodiscard]] double dot(const Vec2D &vec) const; // Returns dot product

    // Operations with numbers
    [[nodiscard]] Vec2D operator*(double number) const;
    [[nodiscard]] Vec2D operator/(double number) const;

    // Other useful methods
    [[nodiscard]] double sqrAbs() const; // Returns squared vector length
    [[nodiscard]] double abs() const; // Returns vector length
    [[nodiscard]] Vec2D normalized() const; // Returns normalized vector without changing
    [[nodiscard]] double length() const;

    static Vec2D Random();
};
