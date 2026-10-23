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

/// Renders viewports of one window into its surface, once per frame. Owns the single
/// window cursor and gives it, in local coordinates, to the topmost viewport under it.
/// Owns the depth target of the surface (layout's depth format, resized with it).
class Render final : public FrameAPI {
public:
    Render(WindowAPI& window, WGPU::Surface& surface, RenderLayout& layout);

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
    WGPU::Surface& surface_;
    RenderLayout& layout_;
    std::unique_ptr<WGPU::Texture> depth_;
    std::vector<Viewport*> viewports_;
    glm::vec2 cursor_{};
    Viewport* hovered_ = nullptr;
    std::function<void(Viewport*)> onHover_;

    void routeCursor(glm::uvec2 surfaceSize);
};
