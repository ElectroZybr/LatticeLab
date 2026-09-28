#pragma once

#include <algorithm>
#include <iterator>
#include <optional>

#include <Lattice/Kernel/NodeFactory.hpp>
#include <Lattice/Kernel/NodeHandlers.hpp>
#include <Lattice/Kernel/Blueprints.hpp>
#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Kernel/NodeQuery.hpp>
#include <Lattice/Kernel/NodeSystem.hpp>
#include <Lattice/Kernel/TreeView.hpp>
#include <Lattice/Tools/TypeName.hpp>


// global types

class ExportsView {
    Lattice::NodeSystem* nodeSystem_ = nullptr;

public:
    class AvailableExports {
    public:
        class Iterator {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = Lattice::VisibleExport;
            using difference_type = std::ptrdiff_t;
            using pointer = const value_type*;
            using reference = const value_type&;

            Iterator() = default;

            reference operator*() const noexcept { return current_; }
            pointer operator->() const noexcept { return &current_; }

            Iterator& operator++() {
                advance();
                return *this;
            }

            Iterator operator++(int) {
                Iterator previous = *this;
                ++*this;
                return previous;
            }

            friend bool operator==(const Iterator& iterator, std::default_sentinel_t) noexcept {
                return iterator.current_.role == Lattice::InvalidRoleId;
            }

        private:
            friend class AvailableExports;

            Iterator(Lattice::NodeSystem* nodeSystem, Lattice::NodeId from)
                : nodeSystem_(nodeSystem),
                  scope_(nodeSystem ? nodeSystem->context.nearestScope(from) : Lattice::InvalidContextScopeId),
                  rootScope_(nodeSystem ? nodeSystem->context.root() : Lattice::InvalidContextScopeId) {
                advance();
            }

            void advance() {
                current_ = {};
                if (!nodeSystem_)
                    return;

                while (nextRole_ < nodeSystem_->context.roleCount()) {
                    const Lattice::RoleId role = nextRole_++;
                    if (!nodeSystem_->context.hasRole(role))
                        continue;

                    const Lattice::ContextResolution resolution =
                        scope_ == Lattice::InvalidContextScopeId
                            ? nodeSystem_->context.resolveInfo(role)
                            : nodeSystem_->context.resolveInfo(scope_, role);
                    if (resolution.state == Lattice::ContextResolutionState::Missing)
                        continue;

                    const Lattice::ContextResolution globalResolution =
                        rootScope_ == Lattice::InvalidContextScopeId
                            ? Lattice::ContextResolution{}
                            : nodeSystem_->context.resolveInfo(rootScope_, role);

                    Lattice::ExportId exportId = Lattice::InvalidExportId;
                    if (resolution.state == Lattice::ContextResolutionState::Resolved) {
                        exportId = nodeSystem_->exports.find(
                            resolution.target,
                            nodeSystem_->context.roleName(role)
                        );
                        if (exportId == Lattice::InvalidExportId)
                            continue;
                    }

                    current_ = {
                        .role = role,
                        .owner = resolution.target,
                        .exportId = exportId,
                        .scope = scope_,
                        .state = resolution.state,
                        .global = (
                            resolution.state == Lattice::ContextResolutionState::Resolved &&
                            globalResolution.state == Lattice::ContextResolutionState::Resolved &&
                            resolution.target == globalResolution.target
                        ) || (
                            resolution.state == Lattice::ContextResolutionState::Ambiguous &&
                            globalResolution.state == Lattice::ContextResolutionState::Ambiguous &&
                            std::ranges::equal(resolution.candidates, globalResolution.candidates)
                        )
                    };
                    return;
                }
            }

            Lattice::NodeSystem* nodeSystem_ = nullptr;
            Lattice::ContextScopeId scope_ = Lattice::InvalidContextScopeId;
            Lattice::ContextScopeId rootScope_ = Lattice::InvalidContextScopeId;
            Lattice::RoleId nextRole_ = 0;
            Lattice::VisibleExport current_;
        };

        Iterator begin() const { return Iterator{nodeSystem_, from_}; }
        std::default_sentinel_t end() const noexcept { return {}; }

    private:
        friend class ExportsView;

        AvailableExports(Lattice::NodeSystem* nodeSystem, Lattice::NodeId from)
            : nodeSystem_(nodeSystem), from_(from) {}

        Lattice::NodeSystem* nodeSystem_ = nullptr;
        Lattice::NodeId from_ = Lattice::InvalidNodeId;
    };

    ExportsView() = default;
    explicit ExportsView(Lattice::NodeSystem& nodeSystem) : nodeSystem_(&nodeSystem) {}

    bool exists() const noexcept { return nodeSystem_ != nullptr; }
    explicit operator bool() const noexcept { return exists(); }

    Lattice::RoleId role(std::string_view name) const {
        return nodeSystem_->context.role(name);
    }

    Lattice::ResolvedExport resolve(Lattice::RoleId role) const {
        return resolveOwner(role, nodeSystem_->context.resolve(role));
    }

    Lattice::ResolvedExport resolve(Lattice::RoleId role, Lattice::NodeId from) const {
        const auto scope = nodeSystem_->context.nearestScope(from);
        const Lattice::NodeId owner = scope == Lattice::InvalidContextScopeId
            ? nodeSystem_->context.resolve(role)
            : nodeSystem_->context.resolve(scope, role);
        return resolveOwner(role, owner);
    }

    Lattice::ResolvedExport resolveExport(Lattice::ExportId id, Lattice::NodeId from) const {
        const Lattice::Export* entry = nodeSystem_->exports.get(id);
        if (!entry)
            return {};

        const Lattice::RoleId role = nodeSystem_->context.findRole(entry->name);
        if (role == Lattice::InvalidRoleId)
            return {};

        const auto scope = nodeSystem_->context.nearestScope(from);
        const Lattice::NodeId owner = scope == Lattice::InvalidContextScopeId
            ? nodeSystem_->context.resolve(role)
            : nodeSystem_->context.resolve(scope, role);
        return owner == entry->owner ? nodeSystem_->exports.resolve(id) : Lattice::ResolvedExport{};
    }

    AvailableExports available(Lattice::NodeId from) const {
        return AvailableExports{nodeSystem_, from};
    }

    std::string_view name(const Lattice::VisibleExport& entry) const {
        return nodeSystem_->context.roleName(entry.role);
    }

    std::string_view name(Lattice::ExportId id) const {
        const Lattice::Export* entry = nodeSystem_->exports.get(id);
        return entry ? std::string_view{entry->name} : std::string_view{};
    }

    std::optional<Lattice::ExportKind> kind(const Lattice::VisibleExport& entry) const {
        const Lattice::Export* exportEntry = nodeSystem_->exports.get(entry.exportId);
        if (!exportEntry)
            return std::nullopt;
        return exportEntry->kind;
    }

    std::optional<Lattice::Value> value(const Lattice::VisibleExport& entry) const {
        if (entry.exportId == Lattice::InvalidExportId)
            return std::nullopt;
        const Lattice::ResolvedExport resolved = nodeSystem_->exports.resolve(entry.exportId);
        if (!resolved.get)
            return std::nullopt;
        return resolved.get(resolved.object);
    }

    std::span<const Lattice::Value> argumentTypes(const Lattice::VisibleExport& entry) const {
        if (entry.exportId == Lattice::InvalidExportId)
            return {};
        return nodeSystem_->exports.resolve(entry.exportId).argumentTypes;
    }

    size_t requiredArguments(const Lattice::VisibleExport& entry) const {
        if (entry.exportId == Lattice::InvalidExportId)
            return 0;
        return nodeSystem_->exports.resolve(entry.exportId).requiredArguments;
    }

    std::span<const Lattice::NodeId> candidates(const Lattice::VisibleExport& entry) const {
        const Lattice::ContextResolution resolution =
            entry.scope == Lattice::InvalidContextScopeId
                ? nodeSystem_->context.resolveInfo(entry.role)
                : nodeSystem_->context.resolveInfo(entry.scope, entry.role);
        return resolution.candidates;
    }

private:
    Lattice::ResolvedExport resolveOwner(
        Lattice::RoleId role,
        Lattice::NodeId owner
    ) const {
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
    Ref<U> addLocal(std::string_view instance = Lattice::DefaultInstanceName) {
        const auto api = nodeSystem_->blueprints.id<U>();
        const auto id = nodeSystem_->factory.addLocal(owner_, target_, api, instance);
        return Ref<U>{static_cast<U*>(nodeSystem_->query.resolve(id, api))};
    }

    template<class U>
    Ref<U> addLocal(std::string_view instance, const typename U::Desc& desc) {
        const auto api = nodeSystem_->blueprints.id<U>();
        const auto id = nodeSystem_->factory.addLocal(owner_, target_, api, instance, &desc);
        return Ref<U>{static_cast<U*>(nodeSystem_->query.resolve(id, api))};
    }

    template<class U>
    Ref<U> addShare(std::string_view instance = Lattice::DefaultInstanceName) {
        const auto api = nodeSystem_->blueprints.id<U>();
        const auto id = nodeSystem_->factory.addShare(owner_, target_, api, instance);
        return Ref<U>{static_cast<U*>(nodeSystem_->query.resolve(id, api))};
    }

    template<class U>
    Ref<U> addShare(std::string_view instance, const typename U::Desc& desc) {
        const auto api = nodeSystem_->blueprints.id<U>();
        const auto id = nodeSystem_->factory.addShare(owner_, target_, api, instance, &desc);
        return Ref<U>{static_cast<U*>(nodeSystem_->query.resolve(id, api))};
    }

    Ref<T> ref() const {
        return Ref<T>{
            static_cast<T*>(nodeSystem_->query.resolve(target_, nodeSystem_->blueprints.id<T>()))
        };
    }

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
    // NodeBuild(const NodeBuild&) = delete;
    // NodeBuild& operator=(const NodeBuild&) = delete;

    // NodeBuild(NodeBuild&&) = default;
    // NodeBuild& operator=(NodeBuild&&) = delete;

    template<class T> 
    Ref<T> add(std::string_view instance = Lattice::DefaultInstanceName) {
        auto api = nodeSystem_.blueprints.find(Lattice::typeKey<T>());
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
    void share() {
        nodeSystem_.factory.share(id_, nodeSystem_.blueprints.id<API>());
    }

    template<class API> 
    Children<API> addImpls() {
        std::vector<API*> result;
        auto api = nodeSystem_.blueprints.find(Lattice::typeKey<API>());

        for (auto impl : nodeSystem_.blueprints.getImpls(api)) {
            const std::string name = nodeSystem_.blueprints.require(impl).name;
            auto node = nodeSystem_.factory.component(id_, impl, name);
            result.push_back(static_cast<API*>(nodeSystem_.factory.resolve(node, api)));
        }

        return Children<API>{std::move(result)};
    }

    template<class T>
    MountBuild<T> mount() {
        const auto api = nodeSystem_.blueprints.id<T>();
        const auto target = nodeSystem_.query.shared(id_, api);
        return MountBuild<T>{nodeSystem_, id_, target};
    }

    template<typename T>
    Lattice::ExportId param(std::string_view name, T& value) {
        const auto id = nodeSystem_.exports.param(id_, name, value);
        registerLocalExport(name);
        return id;
    }

    template<typename T>
    Lattice::ExportId globalParam(std::string_view name, T& value) {
        const auto id = nodeSystem_.exports.param(id_, name, value);
        registerGlobalExport(name);
        return id;
    }

    template<typename... Args, typename F>
    Lattice::ExportId action(std::string_view name, F&& callback) {
        const auto id = nodeSystem_.exports.action<Args...>(id_, name, std::forward<F>(callback));
        registerLocalExport(name);
        return id;
    }

    template<typename... Args, typename F>
    Lattice::ExportId globalAction(std::string_view name, F&& callback) {
        const auto id = nodeSystem_.exports.action<Args...>(id_, name, std::forward<F>(callback));
        registerGlobalExport(name);
        return id;
    }

    void alias(std::string_view name, Lattice::ExportId target) {
        nodeSystem_.exports.alias(id_, name, target);
        registerLocalExport(name);
    }

    void globalAlias(std::string_view name, Lattice::ExportId target) {
        nodeSystem_.exports.alias(id_, name, target);
        registerGlobalExport(name);
    }

    Lattice::TreeView tree() const { return Lattice::TreeView{nodeSystem_}; }

    template<class T>
    Ref<T> ancestor() const {
        const auto api = nodeSystem_.blueprints.id<T>();

        Lattice::NodeId current = nodeSystem_.registry.require(id_).parent;

        while (current != Lattice::InvalidNodeId) {
            if (auto* ptr = static_cast<T*>(nodeSystem_.query.resolve(current, api)))
                return Ref<T>{ptr};

            current = nodeSystem_.registry.require(current).parent;
        }

        throw Lattice::Exception<NodeBuild>("Ancestor '{}' not found for node #{}", Lattice::typeKey<T>(), id_);
    }

    // helpers
    Lattice::NodeId id() const noexcept { return id_; }
    std::string_view name() const { return nodeSystem_.registry.require(id_).name; }
    Lattice::NodeId parent() const { return nodeSystem_.registry.require(id_).parent; }
    Lattice::NodeKind kind() const { return nodeSystem_.registry.require(id_).kind; }
    Lattice::BlueprintId blueprint() const { return nodeSystem_.registry.require(id_).bp; }
    Lattice::BlueprintId implementation() const { return nodeSystem_.registry.require(id_).object.bp; }

private:
    void registerLocalExport(std::string_view name) {
        const Lattice::ContextScopeId scope = nodeSystem_.context.createScope(id_);
        nodeSystem_.context.addCandidate(scope, nodeSystem_.context.role(name), id_);
    }

    void registerGlobalExport(std::string_view name) {
        Lattice::NodeId root = id_;
        while (nodeSystem_.registry.require(root).parent != Lattice::InvalidNodeId)
            root = nodeSystem_.registry.require(root).parent;

        const Lattice::ContextScopeId scope = nodeSystem_.context.createScope(root);
        nodeSystem_.context.addCandidate(scope, nodeSystem_.context.role(name), id_);
    }
};


class NodeConfigure {
    Lattice::NodeId id_;
    Lattice::NodeSystem& nodeSystem_;

public:
    NodeConfigure(Lattice::NodeId id, Lattice::NodeSystem& nodeSystem)
        : id_(id), nodeSystem_(nodeSystem){}

    template<typename T>
    Focus<T> focus(std::string_view role = Lattice::typeKey<T>()) {
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
        const auto found = findId(Lattice::typeKey<T>(), instance);
        return Ref<T>{found == Lattice::InvalidNodeId ? nullptr : resolve<T>(found)};
    }

    template<class T>
    Ref<T> require(std::string_view instance = Lattice::DefaultInstanceName) const {
        return Ref<T>{resolve<T>(requireId(Lattice::typeKey<T>(), instance))};
    }

    template<class T>
    std::optional<Slot<T>> findSlot(std::string_view instance = Lattice::DefaultInstanceName) const {
        const Lattice::NodeId found = findId(Lattice::typeKey<T>(), instance);
        if (found == Lattice::InvalidNodeId || nodeSystem_.registry.require(found).kind != Lattice::NodeKind::Slot)
            return std::nullopt;
        return Slot<T>{nodeSystem_.factory, nodeSystem_.query, nodeSystem_.blueprints, found};
    }

    template<class T>
    Slot<T> requireSlot(std::string_view instance = Lattice::DefaultInstanceName) const {
        auto slot = findSlot<T>(instance);
        if (!slot)
            throw Lattice::Exception<NodeConfigure>("Slot '{}' with instance '{}' not found", Lattice::typeKey<T>(), instance);
        return *slot;
    }

    template<class T>
    T* resolve(Lattice::NodeId id) const {
        return static_cast<T*>(nodeSystem_.query.resolve(id, Lattice::typeKey<T>()));
    }

    ExportsView exports() {
        return ExportsView{nodeSystem_};
    }

    Lattice::TreeView tree() const { return Lattice::TreeView{nodeSystem_}; }

    // helpers
    Lattice::NodeId id() const noexcept { return id_; }
    std::string_view name() const { return nodeSystem_.registry.require(id_).name; }
    Lattice::NodeId parent() const { return nodeSystem_.registry.require(id_).parent; }
    Lattice::NodeKind kind() const { return nodeSystem_.registry.require(id_).kind; }
    Lattice::BlueprintId blueprint() const { return nodeSystem_.registry.require(id_).bp; }
    Lattice::BlueprintId implementation() const { return nodeSystem_.registry.require(id_).object.bp; }
};
