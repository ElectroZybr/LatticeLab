#pragma once

#include <glm/glm.hpp>
#include <Actions/InputTargetAPI.hpp>

/// Updates a camera from accumulated input once per frame. The input is common to all
/// controllers (exported once as runtime params); each one interprets what it needs.
/// A controller receives keybinds while it is the input target.
class TransformController : public InputTargetAPI {
public:
    virtual ~TransformController() = default;
    virtual void update() {}

    glm::vec3 move{};
    glm::vec2 look{}, pan{};
    float zoom = 0.0f;
};
