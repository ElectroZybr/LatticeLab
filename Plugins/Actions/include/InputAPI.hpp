#pragma once

#include <Lattice/Kernel/Consts.hpp>
#include <Lattice/Tools/ObjectRegistry.hpp>

#include <glm/glm.hpp>


using TriggerId = uint32_t;
inline constexpr TriggerId InvalidTriggerId = std::numeric_limits<TriggerId>::max();
enum class InputKind : uint8_t { Button, Axis, Axis2 };

struct Trigger {
    std::string name;
    InputKind kind = InputKind::Button;
};

using TriggerRegistry = Lattice::ObjectRegistry<Trigger, TriggerId, std::string>;


class InputAPI : public Lattice::Component {
public:
    virtual void registerTriggers(TriggerRegistry& triggers) = 0;

    virtual bool down(TriggerId trigger) const = 0;
    virtual bool pressed(TriggerId trigger) const = 0;
    virtual bool released(TriggerId trigger) const = 0;

    virtual double axis(TriggerId trigger) const { return 0.0; }
    virtual glm::vec2 axis2(TriggerId trigger) const { return {}; }
};
