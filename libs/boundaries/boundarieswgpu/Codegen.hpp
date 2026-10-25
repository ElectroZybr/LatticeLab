#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <boundaries/Program.hpp>

namespace Boundaries::Wgpu {

/// WGSL of `fn <name>(p: vec3<f32>) -> f32`: the distance of a program (> 0 in the
/// region), with the sequence of operations fixed in code instead of interpreted — no
/// stack array and no dispatch on the operation; parameters and transforms are still read
/// from `geometryCode`, so new parameters (animation) need no new shader, a new structure
/// does. Appended after geometrySource(), which it uses.
std::string generateDistance(const Program& program, std::string_view name);

/// The same distance through the interpreting machine (geometryDistance): the reference
/// the generated code is compared with.
std::string interpretedDistance(const Program& program, std::string_view name);

/// How a shader gets the distance of a program: `fn <name>(p) -> f32` after geometrySource().
using DistanceSource = std::string (*)(const Program& program, std::string_view name);

/// What a generated shader depends on: the operations in order. Equal structures share a
/// shader; anything else (parameters, transforms, materials) is data.
std::vector<Op> structure(const Program& program);

}
