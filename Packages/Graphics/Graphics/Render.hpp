#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <WGPU/Device.hpp>
#include <WGPU/Resources.hpp>
#include <Shell/FrameAPI.hpp>
#include <Shell/WindowAPI.hpp>

#include "RenderLayout.hpp"
#include "TransformController.hpp"
#include "Viewport.hpp"

/// Renders viewports of one window into its surface, once per frame. Owns the surface,
/// its depth target and the pipeline layout of the window (RenderLayout) — internal
/// resources, not tree nodes — and the single window cursor, given in local
/// coordinates to the topmost viewport under it.
class Render final : public FrameAPI {
public:
    Render(WGPU::Device& device, WindowAPI& window);

    /// What viewports and scene objects of this window build their pipelines on.
    /// They use it while constructing; their resources do not refer to it later.
    RenderLayout& layout() noexcept { return layout_; }

    void frame(float dt) override;

    /// Viewports drawn in this surface, by layer; composition assigns the current set.
    void setViewports(std::vector<Viewport*> viewports);

    /// Cursor in surface pixels; exported as the window's cursor parameter.
    glm::vec2& cursor() noexcept { return cursor_; }

    /// Called when the viewport under the cursor changes (nullptr: none).
    void onHover(std::function<void(Viewport*)> callback) { onHover_ = std::move(callback); }

private:
    WGPU::Device& device_;
    WindowAPI& window_;
    WGPU::Surface surface_;
    RenderLayout layout_;
    std::unique_ptr<WGPU::Texture> depth_;
    std::vector<Viewport*> viewports_;
    glm::vec2 cursor_{};
    Viewport* hovered_ = nullptr;
    std::function<void(Viewport*)> onHover_;

    void routeCursor(glm::uvec2 surfaceSize);
};
