#pragma once

#include <vector>

#include <glm/glm.hpp>

namespace Generators {

struct Bounds {
    glm::vec3 min = glm::vec3(0.0f);
    glm::vec3 max = glm::vec3(0.0f);
};

class Region {
public:
    virtual ~Region() = default;

    glm::vec3 center = glm::vec3(0.0f);

    [[nodiscard]] virtual bool contains(glm::vec3 point) const = 0;
    [[nodiscard]] virtual Bounds bounds() const = 0;
};

class Rectangle final : public Region {
public:
    glm::vec3 boxSize = glm::vec3(0.0f);

    [[nodiscard]] bool contains(glm::vec3 point) const override;
    [[nodiscard]] Bounds bounds() const override;
};

class Sphere final : public Region {
public:
    float sphereRadius = 0.0f;

    [[nodiscard]] bool contains(glm::vec3 point) const override;
    [[nodiscard]] Bounds bounds() const override;
};

class Cylinder final : public Region {
public:
    float baseRadius = 0.0f;
    float cylinderHeight = 0.0f;

    [[nodiscard]] bool contains(glm::vec3 point) const override;
    [[nodiscard]] Bounds bounds() const override;
};

class Capsule final : public Region {
public:
    float capsuleRadius = 0.0f;
    float capsuleHeight = 0.0f;

    [[nodiscard]] bool contains(glm::vec3 point) const override;
    [[nodiscard]] Bounds bounds() const override;
};

class Torus final : public Region {
public:
    float majorRadius = 0.0f;
    float tubeRadius = 0.0f;

    [[nodiscard]] bool contains(glm::vec3 point) const override;
    [[nodiscard]] Bounds bounds() const override;
};

class PolygonPrism final : public Region {
public:
    std::vector<glm::vec2> polygonPoints;
    float minZ = 0.0f;
    float maxZ = 0.0f;

    [[nodiscard]] bool contains(glm::vec3 point) const override;
    [[nodiscard]] Bounds bounds() const override;
};

class TrianglePyramid final : public Region {
public:
    float baseCircumradius = 0.0f;
    float pyramidHeight = 0.0f;

    [[nodiscard]] bool contains(glm::vec3 point) const override;
    [[nodiscard]] Bounds bounds() const override;
};

class TriangleBiPyramid final : public Region {
public:
    float baseCircumradius = 0.0f;
    float bipyramidHeight = 0.0f;

    [[nodiscard]] bool contains(glm::vec3 point) const override;
    [[nodiscard]] Bounds bounds() const override;
};

} // namespace Generators
