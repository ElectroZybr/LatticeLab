#include "Shape.hpp"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <Lattice/tools/Exception.hpp>

namespace Boundaries {

using Error = Lattice::Exception<Shape>;

namespace {

Instruction primitive(Op op, glm::vec4 first, glm::vec4 second = {}) {
    Instruction instruction{.op = op};
    instruction.params[0] = first;
    instruction.params[1] = second;
    return instruction;
}

void requirePositive(float value, const char* what) {
    if (!std::isfinite(value) || value <= 0.0f)
        throw Error("{} must be positive and finite, got {}", what, value);
}

glm::mat4 rows(const Instruction& instruction) {
    glm::mat4 map(1.0f);
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 4; ++column)
            map[column][row] = instruction.local[row][column];
    return map;
}

void setRows(Instruction& instruction, const glm::mat4& map) {
    for (int row = 0; row < 3; ++row)
        instruction.local[row] = {map[0][row], map[1][row], map[2][row], map[3][row]};
}

}

Shape::Shape(Instruction instruction) : code_{instruction} {}

Shape Shape::sphere(float radius) {
    requirePositive(radius, "sphere radius");
    return Shape{primitive(Op::Sphere, {radius, 0, 0, 0})};
}

Shape Shape::box(glm::vec3 halfExtents) {
    for (int axis = 0; axis < 3; ++axis)
        requirePositive(halfExtents[axis], "box half extent");
    return Shape{primitive(Op::Box, glm::vec4(halfExtents, 0))};
}

Shape Shape::capsule(glm::vec3 a, glm::vec3 b, float radius) {
    requirePositive(radius, "capsule radius");
    return Shape{primitive(Op::Capsule, glm::vec4(a, 0), glm::vec4(b, radius))};
}

Shape Shape::cylinder(float radius, float halfHeight) {
    requirePositive(radius, "cylinder radius");
    requirePositive(halfHeight, "cylinder half height");
    return Shape{primitive(Op::Cylinder, {radius, halfHeight, 0, 0})};
}

Shape Shape::halfSpace(glm::vec3 normal, float offset) {
    const float length = glm::length(normal);
    requirePositive(length, "half-space normal length");
    return Shape{primitive(Op::HalfSpace, glm::vec4(normal / length, offset / length))};
}

Shape Shape::infiniteCylinder(float radius) {
    requirePositive(radius, "cylinder radius");
    return Shape{primitive(Op::InfiniteCylinder, {radius, 0, 0, 0})};
}

Shape Shape::binary(Op op, Shape a, Shape b, float parameter) {
    a.code_.insert(a.code_.end(), b.code_.begin(), b.code_.end());
    a.code_.push_back(primitive(op, {parameter, 0, 0, 0}));
    return a;
}

Shape operator|(Shape a, Shape b) { return Shape::binary(Op::Union, std::move(a), std::move(b)); }
Shape operator&(Shape a, Shape b) { return Shape::binary(Op::Intersect, std::move(a), std::move(b)); }
Shape operator-(Shape a, Shape b) { return Shape::binary(Op::Subtract, std::move(a), std::move(b)); }

Shape Shape::smoothUnion(Shape a, Shape b, float k) {
    requirePositive(k, "smooth union radius");
    return binary(Op::SmoothUnion, std::move(a), std::move(b), k);
}

Shape Shape::unary(Op op, float parameter) && {
    code_.push_back(primitive(op, {parameter, 0, 0, 0}));
    return std::move(*this);
}

Shape Shape::rounded(float radius) && {
    requirePositive(radius, "rounding radius");
    return std::move(*this).unary(Op::Round, radius);
}

Shape Shape::shell(float halfThickness) && {
    requirePositive(halfThickness, "shell half thickness");
    return std::move(*this).unary(Op::Shell, halfThickness);
}

Shape Shape::transformed(const glm::mat4& inverse, float scale) && {
    for (auto& instruction : code_) {
        if (isPrimitive(instruction.op)) {
            setRows(instruction, rows(instruction) * inverse);
            instruction.scale *= scale;
        } else if (instruction.op != Op::Union && instruction.op != Op::Intersect && instruction.op != Op::Subtract) {
            // Blend radius, rounding and thickness are distances: they scale with the shape.
            instruction.params[0].x *= scale;
        }
    }
    return std::move(*this);
}

Shape Shape::translated(glm::vec3 offset) && {
    return std::move(*this).transformed(glm::translate(glm::mat4(1.0f), -offset), 1.0f);
}

Shape Shape::rotated(glm::quat rotation) && {
    return std::move(*this).transformed(glm::mat4_cast(glm::inverse(glm::normalize(rotation))), 1.0f);
}

Shape Shape::scaled(float factor) && {
    requirePositive(factor, "scale factor");
    return std::move(*this).transformed(glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / factor)), factor);
}

Shape Shape::material(uint32_t index) && {
    for (auto& instruction : code_)
        if (isPrimitive(instruction.op))
            instruction.material = index;
    return std::move(*this);
}

Shape Shape::motion(uint32_t index) && {
    for (auto& instruction : code_)
        if (isPrimitive(instruction.op))
            instruction.motion = index;
    return std::move(*this);
}

Program Shape::compile() const {
    Program program{.code = code_};
    uint32_t depth = 0;
    for (const auto& instruction : code_) {
        if (isPrimitive(instruction.op)) {
            program.depth = std::max(program.depth, ++depth);
        } else if (isBinary(instruction.op)) {
            if (depth < 2)
                throw Error("operation needs two shapes on the stack");
            --depth;
        } else if (depth < 1) {
            throw Error("operation needs a shape on the stack");
        }
    }
    if (depth != 1)
        throw Error("program leaves {} shapes on the stack, expected one", depth);
    return program;
}

}
