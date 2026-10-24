#pragma once

#include <string_view>

#include <webgpu/webgpu.h>

#include "Types.hpp"

namespace Wgpu {

/// Conversions to the wgpu-native C API, for code that builds on these wrappers
/// (render passes and pipelines live with the renderer).
WGPUTextureFormat nativeFormat(TextureFormat format);

inline WGPUStringView nativeString(std::string_view value) {
    return {value.data(), value.size()};
}

}
