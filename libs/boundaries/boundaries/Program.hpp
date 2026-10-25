#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

namespace Boundaries {

/// Operation codes of a geometry program. Primitives push a distance, operations
/// combine the top of the stack. A new primitive or operation is a new code plus its
/// evaluation (CPU here, WGSL in the GPU module); nothing else changes.
enum class Op : uint32_t {
    // primitives: params in local coordinates (see Instruction)
    Sphere,            // params[0].x radius
    Box,               // params[0].xyz half extents
    Capsule,           // params[0].xyz a, params[1].xyz b, params[1].w radius
    Cylinder,          // params[0].x radius, params[0].y half height; axis local y
    HalfSpace,         // params[0].xyz unit normal, params[0].w offset: region dot(n, p) <= offset
    InfiniteCylinder,  // params[0].x radius; axis local y, unbounded
    // operations (pop 2, push 1)
    Union,
    Intersect,
    Subtract,          // first minus second
    SmoothUnion,       // params[0].x blend radius k
    // operations (pop 1, push 1)
    Round,             // params[0].x radius
    Shell,             // params[0].x half thickness
};

constexpr bool isPrimitive(Op op) noexcept { return op <= Op::InfiniteCylinder; }
constexpr bool isBinary(Op op) noexcept { return op >= Op::Union && op <= Op::SmoothUnion; }

/// One instruction: fixed size and plain data, so the program is one buffer on the
/// CPU and on the GPU alike.
struct Instruction {
    Op op;
    uint32_t material = 0;   // surface property table index (primitives)
    uint32_t motion = 0;     // rigid motion index for surface velocity (primitives)
    float scale = 1.0f;      // primitives: world distance = scale × local distance
    glm::vec4 local[3] = {   // primitives: rows of the world → local affine map
        {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}};
    glm::vec4 params[2] = {};
};
static_assert(sizeof(Instruction) == 96, "Instruction is a GPU record: keep it 96 bytes");

/// A compiled geometry: postfix instructions and the stack depth they need.
struct Program {
    std::vector<Instruction> code;
    uint32_t depth = 0;
};

}
