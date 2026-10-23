#pragma once

#include <vector>

#include <WGPU/Device.hpp>
#include <WGPU/Resources.hpp>
#include <Shell/FrameAPI.hpp>
#include <Shell/WindowAPI.hpp>

#include "TransformController.hpp"
#include "Viewport.hpp"

/// Renders viewports of one window into its surface, once per frame. Owns the single
/// window cursor and gives it, in local coordinates, to the topmost viewport under it.
class Render final : public FrameAPI {
public:
    Render(WGPU::Device& device, WindowAPI& window, WGPU::Surface& surface,
           std::vector<Viewport*> viewports, std::vector<TransformController*> controllers = {});

    void frame(float dt) override;

    /// Cursor in surface pixels; exported as the window's cursor parameter.
    glm::vec2& cursor() noexcept { return cursor_; }

private:
    WGPU::Device& device_;
    WindowAPI& window_;
    WGPU::Surface& surface_;
    std::vector<Viewport*> viewports_;
    std::vector<TransformController*> controllers_;
    glm::vec2 cursor_{};

    void routeCursor(glm::uvec2 surfaceSize);
};
