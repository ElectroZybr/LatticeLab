#pragma once

#include <glm/glm.hpp>
#include <Lattice/Kernel/Consts.hpp>

class TransformController : public Lattice::Component {
public:
    virtual ~TransformController() = default;
    virtual void update() {}
};
