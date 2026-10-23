#pragma once

#include <glm/glm.hpp>

/// Updates a camera from accumulated input once per frame.
class TransformController {
public:
    virtual ~TransformController() = default;
    virtual void update() {}
};
