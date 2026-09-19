#pragma once

#include <glm/glm.hpp>
#include "Lattice/Kernel/Component.hpp"
#include "Lattice/Kernel/Node.hpp"

#include "Camera.hpp"

class Viewport : public Lattice::Component {
public:
    void configure(Lattice::Node& node) {
        node.add<Camera>("MainCamera");
    }

    virtual ~Viewport() = default;

    glm::uvec2 size() const noexcept { return size_; }
    void setSize(glm::uvec2 size) noexcept { size_ = size; }

    void render() {
        
    }

private:
    glm::uvec2 size_{1280, 720};
};