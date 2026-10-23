#include "RenderLayout.hpp"

#include <array>

namespace {
constexpr std::array<WGPU::LayoutEntry, 1> cameraEntries{{{.binding = 0, .type = WGPU::BindingType::Uniform}}};
}

RenderLayout::RenderLayout(WGPU::Device& device, WGPU::TextureFormat colorFormat, WGPU::TextureFormat depthFormat)
    : device_(device), colorFormat_(colorFormat), depthFormat_(depthFormat),
      camera_(device, cameraEntries, "camera") {}
