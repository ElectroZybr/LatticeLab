#pragma once

#include <glm/glm.hpp>
#include <Lattice/Kernel/Consts.hpp>

#include <GPU/include/CommandList.hpp>

enum class ViewportSizeMode {
    Fixed,
    Fill
};

class Viewport : public Lattice::Component {
public:
    virtual ~Viewport() = default;
    virtual void render(GPU::RenderPass& pass, glm::uvec2 surfaceSize) = 0;

    glm::uvec2 position() const noexcept { return position_; }
    void setPosition(glm::uvec2 position) noexcept { position_ = position; }

    glm::uvec2 size() const noexcept { return size_; }
    void setSize(glm::uvec2 size) noexcept {
        size_ = size;
        sizeMode_ = ViewportSizeMode::Fixed;
    }

    void fitSurface() noexcept { sizeMode_ = ViewportSizeMode::Fill; }

    glm::vec2 cursor() const noexcept { return cursor_; }
    void setCursor(glm::vec2 cursor) noexcept { cursor_ = cursor; }

protected:
    bool resolveRect(glm::uvec2 surfaceSize, GPU::Rect& rect) const noexcept;
    glm::vec2& cursorValue() noexcept { return cursor_; }

private:
    glm::uvec2 position_{};
    glm::uvec2 size_{1280, 720};
    ViewportSizeMode sizeMode_ = ViewportSizeMode::Fill;
    glm::vec2 cursor_{};
};
