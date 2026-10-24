#pragma once

#include <string_view>

namespace Boundaries::Wgpu {

/// WGSL of the geometry machine (Geometry.wgsl), for shaders outside this module that
/// evaluate a program (drawing the geometry). Prepend it to a shader that declares
///     @group(G) @binding(B) var<storage, read> geometryCode: array<Instruction>;
/// and calls sampleGeometry / geometryDistance.
std::string_view geometrySource();

}
