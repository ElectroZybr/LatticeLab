#pragma once

#include <vector>

#include <WGPU/Device.hpp>
#include <WGPU/Resources.hpp>
#include <Shell/FrameAPI.hpp>
#include <Shell/WindowAPI.hpp>

#include "TransformController.hpp"
#include "Viewport.hpp"

/// Renders viewports of one window into its surface, once per frame.
class Render final : public FrameAPI {
public:
    Render(WGPU::Device& device, WindowAPI& window, WGPU::Surface& surface,
           std::vector<Viewport*> viewports, std::vector<TransformController*> controllers = {});

    void frame(float dt) override;

private:
    WGPU::Device& device_;
    WindowAPI& window_;
    WGPU::Surface& surface_;
    std::vector<Viewport*> viewports_;
    std::vector<TransformController*> controllers_;
};
