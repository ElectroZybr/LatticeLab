#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Program.hpp"

namespace Boundaries {

/**
 * A region of space built from primitives, booleans, blends and transforms.
 *
 * A shape describes where particles may be (the inside of a tube, a box minus a
 * mixer); its surface is the wall. It is a value: combining shapes concatenates their
 * postfix code, transforms fold into each primitive's world → local map, so the
 * compiled program has no transform instructions.
 *
 * Transforms are rigid plus uniform scale: distances stay exact distances (smooth
 * blends give a bound, still fine for normals and soft forces).
 */
class Shape final {
public:
    static Shape sphere(float radius);
    static Shape box(glm::vec3 halfExtents);
    static Shape capsule(glm::vec3 a, glm::vec3 b, float radius);
    /// Capped cylinder along local y, centered at the origin.
    static Shape cylinder(float radius, float halfHeight);
    /// Region dot(normal, p) <= offset.
    static Shape halfSpace(glm::vec3 normal, float offset);
    /// Unbounded cylinder along local y (for periodic channels).
    static Shape infiniteCylinder(float radius);

    friend Shape operator|(Shape a, Shape b);   // union
    friend Shape operator&(Shape a, Shape b);   // intersection
    friend Shape operator-(Shape a, Shape b);   // a minus b
    static Shape smoothUnion(Shape a, Shape b, float k);

    Shape rounded(float radius) &&;
    /// Thin layer around the surface, `halfThickness` to each side.
    Shape shell(float halfThickness) &&;

    Shape translated(glm::vec3 offset) &&;
    Shape rotated(glm::quat rotation) &&;
    Shape scaled(float factor) &&;

    /// Surface property / rigid motion of every primitive of this shape.
    Shape material(uint32_t index) &&;
    Shape motion(uint32_t index) &&;

    Program compile() const;
    const std::vector<Instruction>& code() const noexcept { return code_; }

private:
    explicit Shape(Instruction primitive);
    static Shape binary(Op op, Shape a, Shape b, float parameter = 0.0f);
    Shape unary(Op op, float parameter) &&;
    /// Apply world map `inverse` (new world → old world) to every primitive.
    Shape transformed(const glm::mat4& inverse, float scale) &&;

    std::vector<Instruction> code_;
};

}
