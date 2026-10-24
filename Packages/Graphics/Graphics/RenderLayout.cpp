#include "RenderLayout.hpp"

#include <array>

namespace {
constexpr std::array<Wgpu::LayoutEntry, 1> cameraEntries{{{.binding = 0, .type = Wgpu::BindingType::Uniform}}};
}

RenderLayout::RenderLayout(Wgpu::Device& device, Wgpu::TextureFormat colorFormat, Wgpu::TextureFormat depthFormat)
    : device_(device), colorFormat_(colorFormat), depthFormat_(depthFormat),
      camera_(device, cameraEntries, "camera") {}
