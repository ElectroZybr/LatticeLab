#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include <Lattice/Tools/ObjectRegistry.hpp>
#include <Lattice/Kernel/Consts.hpp>
#include <Lattice/Kernel/Exports.hpp>
#include <Lattice/Kernel/NodeViews.hpp>

#include "InputAPI.hpp"

namespace Lattice { class Node; }

enum class ActionMode { OnPress, OnHold, OnRelease };
enum class Target { Action, Toggle, Add };

class ActionRouter final : public Lattice::SubsystemAPI {
private:
    struct Binding;

    struct BindKey {
        Lattice::RoleId role;
        TriggerId source;
        std::vector<TriggerId> modifiers;

        bool operator==(const BindKey&) const = default;
    };

    struct BindKeyHash {
        size_t operator()(const BindKey& key) const noexcept {
            size_t h = std::hash<uint32_t>{}(key.role);
            h ^= std::hash<uint32_t>{}(key.source) + 0x9e3779b9 + (h << 6) + (h >> 2);
            for (auto id : key.modifiers)
                h ^= std::hash<uint32_t>{}(id) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    using BindId = uint32_t;
    using BindRegistry = Lattice::ObjectRegistry<Binding, BindId, BindKey, BindKeyHash>;

    struct TriggerChain {
        TriggerId source = InvalidTriggerId;
        std::vector<TriggerId> modifiers;
    };

    static bool sameTrigger(const TriggerChain& a, const TriggerChain& b) { return a.source == b.source && a.modifiers == b.modifiers; }

    enum BindingFlags : uint8_t {
        WasDown  = 1 << 0,
        Down     = 1 << 1,
        Pressed  = 1 << 2,
        Released = 1 << 3
    };

    static BindKey bindKey(Lattice::RoleId role, const TriggerChain& chain) { return {role, chain.source, chain.modifiers}; }
    static bool hasFlag(const Binding& b, BindingFlags f) { return (b.flags & f) != 0; }
    static void setFlag(Binding& b, BindingFlags f, bool value) { if (value) b.flags |= f; else b.flags &= ~f; }

    struct Binding {
        bool exists = true;

        Lattice::RoleId role = Lattice::InvalidRoleId;
        TriggerChain trigger;
        ActionMode mode = ActionMode::OnPress;
        Target target = Target::Action;
        double delta = 0;
        uint8_t flags = 0;

        Lattice::ResolvedExport resolved;
    };

public:
    void configure(NodeConfigure branch);
    void tick();
    void registerInput(InputAPI& input);

    void resolve(Binding& binding);
    void resolveBindings();

    std::optional<ActionRouter::TriggerChain> parseTriggerChain(std::string_view expression);

    /// output слой
    void bind(std::string_view verb, std::string_view trigger, ActionMode mode = ActionMode::OnPress);
    void bindToggle(std::string_view param, std::string_view trigger, ActionMode mode = ActionMode::OnPress);
    void bindAdd(std::string_view param, std::string_view trigger, double delta, ActionMode mode = ActionMode::OnPress);
    void bindAxis(std::string_view param, std::string_view trigger);
    void bindAxis2(std::string_view param, std::string_view trigger);

    /// input слой
    bool down(Lattice::RoleId role) const { return any(role, BindingFlags::Down); }
    bool pressed(Lattice::RoleId role) const { return any(role, BindingFlags::Pressed); }
    bool released(Lattice::RoleId role) const { return any(role, BindingFlags::Released); }

    void clearBinds() { bindings_.clear(); }
    
private:
    ExportsView exports;
    Lattice::NodeId context_ = Lattice::InvalidNodeId;
    std::vector<InputAPI*> inputs_;
    BindRegistry bindings_;
    TriggerRegistry triggers_;

    bool any(Lattice::RoleId role, BindingFlags flag) const;
    void upsert(std::string_view verb, std::string_view trigger, ActionMode mode, Target target, double delta);
};
