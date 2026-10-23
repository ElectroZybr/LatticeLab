#pragma once

#include <optional>

#include <glm/glm.hpp>
#include <WGPU/CommandList.hpp>

enum class ViewportSizeMode {
    Fixed,
    Fill
};

/// A rectangular region of a surface that records its own draw commands.
class Viewport {
public:
    virtual ~Viewport() = default;
    virtual void render(WGPU::RenderPass& pass, glm::uvec2 surfaceSize) = 0;

    glm::uvec2 position() const noexcept { return position_; }
    void setPosition(glm::uvec2 position) noexcept { position_ = position; }

    glm::uvec2 size() const noexcept { return size_; }
    void setSize(glm::uvec2 size) noexcept {
        size_ = size;
        sizeMode_ = ViewportSizeMode::Fixed;
    }

    void fitSurface() noexcept { sizeMode_ = ViewportSizeMode::Fill; }

    /// Higher layers are drawn later and receive the cursor first (GUI panels over a scene).
    int layer() const noexcept { return layer_; }
    void setLayer(int layer) noexcept { layer_ = layer; }

    /// Whether a surface position falls inside this viewport's rectangle.
    bool contains(glm::vec2 position, glm::uvec2 surfaceSize) const noexcept;

    /// Cursor in viewport-local coordinates; valid while hovered().
    glm::vec2 cursor() const noexcept { return cursor_; }
    bool hovered() const noexcept { return hovered_; }
    /// Called by the surface owner: local cursor position, or nullopt when the cursor is elsewhere.
    void hover(std::optional<glm::vec2> local) noexcept {
        hovered_ = local.has_value();
        if (local) cursor_ = *local;
    }

protected:
    bool resolveRect(glm::uvec2 surfaceSize, WGPU::Rect& rect) const noexcept;

private:
    glm::uvec2 position_{};
    glm::uvec2 size_{1280, 720};
    ViewportSizeMode sizeMode_ = ViewportSizeMode::Fill;
    int layer_ = 0;
    glm::vec2 cursor_{};
    bool hovered_ = false;
};
