#include "GeometrySource.hpp"

#include <boundaries/Program.hpp>
#include <wgpu/ShaderSource.hpp>

#include "GpuGeometry.hpp"

namespace Boundaries::Wgpu {

namespace {

constexpr unsigned char Geometry[] = {
#include "Geometry.wgsl.h"
};

// The machine mirrors the C++ code: a mismatch fails the build, not a simulation.
constexpr bool sameOp(std::string_view name, Op op) { return ::Wgpu::shaderConstant(Geometry, name) == int64_t(op); }
static_assert(::Wgpu::shaderConstant(Geometry, "GEOMETRY_DEPTH") == MaxDepth);
static_assert(sameOp("OP_SPHERE", Op::Sphere) && sameOp("OP_BOX", Op::Box) && sameOp("OP_CAPSULE", Op::Capsule) &&
              sameOp("OP_CYLINDER", Op::Cylinder) && sameOp("OP_HALF_SPACE", Op::HalfSpace) &&
              sameOp("OP_INFINITE_CYLINDER", Op::InfiniteCylinder) && sameOp("OP_UNION", Op::Union) &&
              sameOp("OP_INTERSECT", Op::Intersect) && sameOp("OP_SUBTRACT", Op::Subtract) &&
              sameOp("OP_SMOOTH_UNION", Op::SmoothUnion) && sameOp("OP_ROUND", Op::Round) && sameOp("OP_SHELL", Op::Shell));
// The machine relies on the order primitives < binary < unary.
static_assert(Op::InfiniteCylinder < Op::Union && Op::SmoothUnion < Op::Round && Op::Round < Op::Shell);

}

std::string_view geometrySource() {
    return ::Wgpu::shaderText(Geometry);
}

}
