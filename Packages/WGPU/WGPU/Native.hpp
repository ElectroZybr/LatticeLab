#pragma once

#include <webgpu/webgpu.h>
#include "Types.hpp"

namespace WGPU {
WGPUTextureFormat nativeFormat(TextureFormat format);
}
