#pragma once

#include <Lattice/Kernel/NodeFactory.hpp>
#include <Lattice/Kernel/NodeHandlers.hpp>
#include <Lattice/Kernel/Blueprints.hpp>
#include <Lattice/Kernel/NodeQuery.hpp>
#include <Lattice/Kernel/NodeSystem.hpp>


// global types

class ExportsView {
    Lattice::NodeSystem* nodeSystem_ = nullptr;

public:
    ExportsView() = default;
    explicit ExportsView(Lattice::NodeSystem& nodeSystem) : nodeSystem_(&nodeSystem) {}

    bool exists() const noexcept { return nodeSystem_ != nullptr; }
    explicit operator bool() const noexcept { return exists(); }

    Lattice::RoleId role(std::string_view name) const {
        return nodeSystem_->context.role(name);
    }

    Lattice::ResolvedExport resolve(Lattice::RoleId role) const {
        const Lattice::NodeId owner = nodeSystem_->context.resolve(role);
        if (owner == Lattice::InvalidNodeId) return {};

        const auto name = nodeSystem_->context.roleName(role);
        const Lattice::ExportId id = nodeSystem_->exports.find(owner, name);
        return id == Lattice::InvalidExportId ? Lattice::ResolvedExport{} : nodeSystem_->exports.resolve(id);
    }
};


template<class T>
class MountBuild {
    Lattice::NodeSystem* nodeSystem_ = nullptr;
    Lattice::NodeId owner_ = Lattice::InvalidNodeId;
    Lattice::NodeId target_ = Lattice::InvalidNodeId;

public:
    MountBuild(Lattice::NodeSystem& nodeSystem, Lattice::NodeId owner, Lattice::NodeId target)
        : nodeSystem_(&nodeSystem), owner_(owner), target_(target) {}

    MountBuild(const MountBuild&) = delete;
    MountBuild& operator=(const MountBuild&) = delete;

    MountBuild(MountBuild&&) = default;
    MountBuild& operator=(MountBuild&&) = delete;

    template<class U>
    Ref<U> add(std::string_view instance = Lattice::DefaultInstanceName) {
        auto bp = nodeSystem_->blueprints.id<U>();
        auto id = nodeSystem_->factory.component(target_, bp, instance);
        return Ref<U>{static_cast<U*>(nodeSystem_->factory.resolve(id, bp))};
    }

    T* get() const {
        return static_cast<T*>(nodeSystem_->query.resolve(target_, nodeSystem_->blueprints.id<T>()));
    }

    T* operator->() const { return get(); }

    Lattice::NodeId owner() const noexcept { return owner_; }
    Lattice::NodeId target() const noexcept { return target_; }
};


class NodeBuild {
    Lattice::NodeId id_;
    Lattice::NodeSystem& nodeSystem_;

public:
    NodeBuild(Lattice::NodeId id, Lattice::NodeSystem& nodeSystem)
        : id_(id), nodeSystem_(nodeSystem) {}

    // move-only
    NodeBuild(const NodeBuild&) = delete;
    NodeBuild& operator=(const NodeBuild&) = delete;

    NodeBuild(NodeBuild&&) = default;
    NodeBuild& operator=(NodeBuild&&) = delete;

    template<class T> 
    Ref<T> add(std::string_view instance = Lattice::DefaultInstanceName) {
        auto api = nodeSystem_.blueprints.find(typeKey<T>());
        auto node = nodeSystem_.factory.component(id_, api, instance);
        return Ref<T>{static_cast<T*>(nodeSystem_.factory.resolve(node, api))};
    }

    template<typename API>
    Slot<API> addSlot(std::string_view instance = Lattice::DefaultInstanceName) {
        const Lattice::NodeId node = nodeSystem_.factory.slot(id_, nodeSystem_.blueprints.id<API>(), instance);
        return Slot<API>{nodeSystem_.factory, nodeSystem_.query, nodeSystem_.blueprints, node};
    }
    
    Lattice::NodeId addFolder(std::string_view name) { return nodeSystem_.factory.folder(id_, name); }

    template<class API> 
    Children<API> addImpls() {
        std::vector<API*> result;
        auto api = nodeSystem_.blueprints.find(typeKey<API>());

        for (auto impl : nodeSystem_.blueprints.getImpls(api)) {
            const std::string name = nodeSystem_.blueprints.require(impl).name;
            auto node = nodeSystem_.factory.component(id_, impl, name);
            result.push_back(static_cast<API*>(nodeSystem_.factory.resolve(node, api)));
        }

        return Children<API>{std::move(result)};
    }

    template<class T>
    MountBuild<T> mount(std::string_view instance = Lattice::DefaultInstanceName) {
        const auto api = nodeSystem_.blueprints.id<T>();
        const auto target = nodeSystem_.query.shared(id_, api, instance);

        if (target == Lattice::InvalidNodeId)
            throw Lattice::Exception("NodeBuild", "Shared '{}' with instance '{}' not found", typeKey<T>(), instance);

        return MountBuild<T>{nodeSystem_, id_, target};
    }

    template<typename T>
    Lattice::ExportId param(std::string_view name, T& value) {
        return nodeSystem_.exports.param(id_, name, value);
    }

    template<typename F>
    Lattice::ExportId action(std::string_view name, F&& callback) {
        return nodeSystem_.exports.action(id_, name, std::forward<F>(callback));
    }

    template<class T>
    Ref<T> ancestor() const {
        const auto api = nodeSystem_.blueprints.id<T>();

        Lattice::NodeId current = nodeSystem_.registry.require(id_).parent;

        while (current != Lattice::InvalidNodeId) {
            if (auto* ptr = static_cast<T*>(nodeSystem_.query.resolve(current, api)))
                return Ref<T>{ptr};

            current = nodeSystem_.registry.require(current).parent;
        }

        throw Exception("NodeBuild", "Ancestor '{}' not found for node #{}", typeKey<T>(), id_);
    }

    // helpers
    Lattice::NodeId id() const noexcept { return id_; }
    std::string_view name() const { return nodeSystem_.registry.require(id_).name; }
    Lattice::NodeId parent() const { return nodeSystem_.registry.require(id_).parent; }
    Lattice::NodeKind kind() const { return nodeSystem_.registry.require(id_).kind; }
    Lattice::BlueprintId blueprint() const { return nodeSystem_.registry.require(id_).bp; }
    Lattice::BlueprintId implementation() const { return nodeSystem_.registry.require(id_).object.bp; }
};


class NodeConfigure {
    Lattice::NodeId id_;
    Lattice::NodeSystem& nodeSystem_;

public:
    NodeConfigure(Lattice::NodeId id, Lattice::NodeSystem& nodeSystem)
        : id_(id), nodeSystem_(nodeSystem){}

    template<typename T>
    Focus<T> focus(std::string_view role = typeKey<T>()) {
        return Focus<T>{nodeSystem_.context, nodeSystem_.query, nodeSystem_.context.nearestScope(id_), nodeSystem_.context.role(role)};
    }

    template<class T>
    std::vector<T*> collect() const {
        std::vector<T*> result;
        const auto api = nodeSystem_.blueprints.id<T>();

        for (Lattice::NodeId id : nodeSystem_.query.collect(id_, api))
            if (auto* ptr = static_cast<T*>(nodeSystem_.query.resolve(id, api)))
                result.push_back(ptr);

        return result;
    }

    template<class T>
    Children<T> children() const {
        std::vector<T*> result;
        const auto api = nodeSystem_.blueprints.id<T>();

        for (Lattice::NodeId child : nodeSystem_.registry.children(id_))
            if (auto* ptr = static_cast<T*>(nodeSystem_.query.resolve(child, api)))
                result.push_back(ptr);

        return Children<T>{std::move(result)};
    }

    Lattice::NodeId findId(std::string_view api, std::string_view instance = Lattice::DefaultInstanceName) const;
    Lattice::NodeId requireId(std::string_view api, std::string_view instance = Lattice::DefaultInstanceName) const;

    template<class T>
    Ref<T> find(std::string_view instance = Lattice::DefaultInstanceName) const {
        const auto found = findId(typeKey<T>(), instance);
        return Ref<T>{found == Lattice::InvalidNodeId ? nullptr : resolve<T>(found)};
    }

    template<class T>
    Ref<T> require(std::string_view instance = Lattice::DefaultInstanceName) const {
        return Ref<T>{resolve<T>(requireId(typeKey<T>(), instance))};
    }

    template<class T>
    std::optional<Slot<T>> findSlot(std::string_view instance = Lattice::DefaultInstanceName) const {
        const Lattice::NodeId found = findId(typeKey<T>(), instance);
        if (found == Lattice::InvalidNodeId || nodeSystem_.registry.require(found).kind != Lattice::NodeKind::Slot)
            return std::nullopt;
        return Slot<T>{nodeSystem_.factory, nodeSystem_.query, nodeSystem_.blueprints, found};
    }

    template<class T>
    Slot<T> requireSlot(std::string_view instance = Lattice::DefaultInstanceName) const {
        auto slot = findSlot<T>(instance);
        if (!slot)
            throw Exception("NodeConfigure", "Slot '{}' with instance '{}' not found", typeKey<T>(), instance);
        return *slot;
    }

    template<class T>
    T* resolve(Lattice::NodeId id) const {
        return static_cast<T*>(nodeSystem_.query.resolve(id, typeKey<T>()));
    }

    ExportsView exports() {
        return ExportsView{nodeSystem_};
    }

    // helpers
    Lattice::NodeId id() const noexcept { return id_; }
    std::string_view name() const { return nodeSystem_.registry.require(id_).name; }
    Lattice::NodeId parent() const { return nodeSystem_.registry.require(id_).parent; }
    Lattice::NodeKind kind() const { return nodeSystem_.registry.require(id_).kind; }
    Lattice::BlueprintId blueprint() const { return nodeSystem_.registry.require(id_).bp; }
    Lattice::BlueprintId implementation() const { return nodeSystem_.registry.require(id_).object.bp; }
};