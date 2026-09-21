#pragma once

#include <Lattice/Kernel/Component.hpp>

#include <string_view>
#include <glm/glm.hpp>

class InputAPI : public Lattice::Component {
public:
    virtual bool down(std::string_view trigger) const = 0;
    virtual bool pressed(std::string_view trigger) const = 0;
    virtual bool released(std::string_view trigger) const = 0;

    virtual double axis(std::string_view trigger) const { return 0.0; }
    virtual glm::vec2 axis2(std::string_view trigger) const { return {}; }
};