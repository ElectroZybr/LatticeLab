#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <Lattice/Kernel/Context.hpp>
#include <Lattice/Kernel/RefSlot.hpp>
#include <Lattice/Kernel/ObjectRegistry.hpp>
#include <Lattice/Kernel/SubsystemAPI.hpp>

#include "InputAPI.hpp"

namespace Lattice { class Node; }

enum class ActionMode { OnPress, OnHold, OnRelease };
enum class Target { Action, Toggle, Add };

class ActionMap final : public SubsystemAPI {
public:
    using BindId = uint32_t;

    struct Binding {
        std::string name;
        bool exists = true;

        Lattice::ContextId slot = Lattice::InvalidContextId;
        std::string trigger;
        ActionMode mode = ActionMode::OnPress;
        Target target = Target::Action;
        double delta = 0;
        bool wasDown = false;
        bool down = false;
        bool pressed = false;
        bool released = false;
    };

    using BindRegistry = Lattice::ObjectRegistry<Binding, BindId, std::string>;

    explicit ActionMap(Lattice::Node& branch) {}

    void configure(Lattice::Node& branch);

    void bind(std::string_view verb, std::string_view trigger, ActionMode mode = ActionMode::OnPress);
    void bindToggle(std::string_view param, std::string_view trigger, ActionMode mode = ActionMode::OnPress);
    void bindAdd(std::string_view param, std::string_view trigger, double delta, ActionMode mode = ActionMode::OnPress);

    void tick();

    bool down(Lattice::ContextId slot) const;
    bool pressed(Lattice::ContextId slot) const;
    bool released(Lattice::ContextId slot) const;

    size_t bindCount() const;
    bool hasBind(std::string_view verb, std::string_view trigger) const;

    void clearBinds();
    
private:
    Ref<Lattice::Context> run_ctx;
    std::vector<InputAPI*> inputs_;
    BindRegistry bindings_;

    static std::string bindName(Lattice::ContextId slot, std::string_view trigger);

    Binding* findBind(Lattice::ContextId slot, std::string_view trigger);
    const Binding* findBind(Lattice::ContextId slot, std::string_view trigger) const;
    bool any(Lattice::ContextId slot, bool Binding::* field) const;
    void upsert(
        std::string_view verb,
        std::string_view trigger,
        ActionMode mode,
        Target target,
        double delta
    );
};
