#pragma once

#include <concepts>
#include <type_traits>
#include <functional>
#include <string>
#include <string_view>
#include <memory>
#include <optional>
#include <vector>
#include <utility>

#include <Lattice/Kernel/ServiceAPI.hpp>
#include <Lattice/Kernel/Requirements.hpp>
#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/RefSlot.hpp>
#include <Lattice/Kernel/Bindings.hpp>
#include <Lattice/Tools/LogStyle.hpp>
#include <Lattice/Tools/Logger.hpp>
#include <Lattice/Tools/LogTree.hpp>
#include <Lattice/Kernel/Context.hpp>


namespace Lattice {

enum class NodeKind : uint8_t {
    Folder,
    Component,
    Slot,
    Binding,
    Mount
};

class Path {
public:
    Path() = default;

    explicit Path(ObjectId id, const ComponentsRegistry& objects) {
        while (id != InvalidObjectId) {
            ids_.push_back(id);
            id = objects.require(id).parent;
        }

        std::ranges::reverse(ids_);
    }

    std::span<const ObjectId> ids() const {
        return ids_;
    }

    bool empty() const {
        return ids_.empty();
    }

    size_t size() const {
        return ids_.size();
    }

    ObjectId operator[](size_t index) const {
        return ids_[index];
    }

    void push(ObjectId id) {
        ids_.push_back(id);
    }

    void pop() {
        ids_.pop_back();
    }

private:
    std::vector<ObjectId> ids_;
};

inline bool unnamedInstance(std::string_view name) noexcept {
    return name.empty() || name == "default";
}

inline std::string_view canonicalInstance(std::string_view name) noexcept {
    return name == "default" ? DefaultInstanceName : name;
}

inline bool sameInstance(std::string_view a, std::string_view b) noexcept {
    return unnamedInstance(a) ? unnamedInstance(b) : a == b;
}

template<typename T>
concept HasConfigure = requires(T& obj, Node& branch) {
    obj.configure(branch);
};

template<typename T, typename D>
concept CreationDescriptor = requires { typename T::Desc; } &&
    std::same_as<std::remove_cvref_t<D>, typename T::Desc>;

class Node {
    static constexpr std::string_view tag = "Node";

    Context& run_ctx;

    ObjectId id = 0;
    Node* parent = nullptr;
    BlueprintId bp = Blueprints::InvalidId;
    BlueprintId implBp = Blueprints::InvalidId;
    void* object = nullptr;
    NodeKind kind = NodeKind::Folder;
    bool configured = false;
    FocusScopeId focusScope = InvalidFocusScopeId;

    std::vector<std::unique_ptr<Node>> children_;

    Node& createNode(std::string_view name, BlueprintId blueprint = Blueprints::InvalidId) {
        auto node = std::make_unique<Node>(run_ctx, this);
        Node& child = *node;
        const auto key = name.empty() ? std::nullopt : std::optional{ObjectKey{std::string(name), id}};
        child.id = run_ctx.objects.create({std::string(name), id, &child}, key, true);
        child.bp = blueprint;
        children_.push_back(std::move(node));
        return child;
    }

    FocusScopeId nearestFocusScope() const {
        const Node* origin = this;
        while (origin && origin->focusScope == InvalidFocusScopeId)
            origin = origin->parent;
        if (!origin)
            throw Exception(tag, "No focus scope for node");
        return origin->focusScope;
    }

    Node* findChild(std::string_view childName) const noexcept {
        for (const auto& child : children_) {
            if (child->name() == childName)
                return child.get();
        }
        return nullptr;
    }

    Node& bindingChild(std::string_view childName) {
        if (Node* child = findChild(childName); child && child->kind == NodeKind::Binding)
            return *child;

        Node& child = createNode(childName);
        child.kind = NodeKind::Binding;
        return child;
    }

    BlueprintId implementation() const noexcept {
        return kind == NodeKind::Slot ? implBp : bp;
    }

    void destroyObject() {
        if (object) {
            if (const auto* blueprint = run_ctx.blueprints.get(implementation()); blueprint && blueprint->meta.destroy)
                blueprint->meta.destroy(object);
        }
        object = nullptr;
        implBp = Blueprints::InvalidId;
        configured = false;
    }

    void configure() {
        if (kind == NodeKind::Mount || !object)
            return;
        if (const auto* blueprint = run_ctx.blueprints.get(implementation()); blueprint && blueprint->meta.configure)
            blueprint->meta.configure(object, *this);
        configured = true;
    }

    void reconfigureFloor() {
        if (!parent)
            return;

        std::vector<Node*> floor;
        for (auto& child : parent->children_) {
            if (child.get() != this && child->configured)
                floor.push_back(child.get());
        }

        for (Node* node : floor) {
            node->configured = false;
            node->configure();
        }
    }

    bool isImplement(BlueprintId apiId) const {
        return run_ctx.blueprints.isA(bp, apiId) || run_ctx.blueprints.isA(implBp, apiId);
    }

    void appendTree(Logger::Tree& tree, size_t depth, ObjectId highlighted) const {
        for (const auto& child : children_) {
            const std::string name = std::string(child->name());
            std::string line;

            switch (child->kind) {
                case NodeKind::Folder:
                    line = std::format("{} <m>F</>", name);
                    break;

                case NodeKind::Component:
                    if (unnamedInstance(name))
                        line = std::format("{} <g>С</>", run_ctx.blueprints.require(child->bp).shortName());
                    else
                        line = std::format("{}<gr>::{}</> <g>С</>", run_ctx.blueprints.require(child->bp).shortName(), name);
                    break;
                
                case NodeKind::Slot:
                    if (unnamedInstance(name))
                        line = run_ctx.blueprints.require(child->bp).shortName();
                    else
                        line = std::format("{}<gr>::{}</>", run_ctx.blueprints.require(child->bp).shortName(), name);
                    line += std::format("<gr>::<c>{}<//> <c>S</>", child->object ? run_ctx.blueprints.require(child->implBp).name : "empty");
                    break;

                case NodeKind::Binding:
                    line = std::format("{} <y>λ</>", name);
                    break;

                case NodeKind::Mount:
                    if (unnamedInstance(name))
                        line = std::format("<m>[&{}]</> <bl>&</>", run_ctx.blueprints.require(child->bp).shortName());
                    else
                        line = std::format("<m>[&{}]</> <bl>&</>", name);
                    break;
            }

            if (child->id == highlighted)
                line = std::format("<b><r>{} 🡸<//>", line);

            line += std::format(" <gr>#{}</>", child->id);
            tree.node(line, depth);
            child->appendTree(tree, depth + 1, highlighted);
        }
    }

    void collectInto(BlueprintId apiId, std::vector<ObjectId>& out) const {
        if (isImplement(apiId))
            out.push_back(id);

        for (const auto& child : children_)
            child->collectInto(apiId, out);
    }

    const Node& rootNode() const {
        const Node* n = this;
        while (n->parent)
            n = n->parent;
        return *n;
    }

    Node& rootNode() {
        Node* n = this;
        while (n->parent)
            n = n->parent;
        return *n;
    }


public:
    Node(Context& run_ctx, Node* parent = nullptr)
        : run_ctx(run_ctx), parent(parent) {
            if (!parent) {
                id = run_ctx.objects.create({"Root", InvalidObjectId, this}, ObjectKey{"Root", InvalidObjectId});
                makeFocusScope();
            }
    }

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator=(Node&&) = delete;

    FocusScopeId getFocusScopeId() const noexcept { return focusScope; }

    FocusScopeId makeFocusScope() {
        return focusScope != InvalidFocusScopeId ? focusScope : makeFocusScope(bp);
    }

    template<class T>
    FocusScopeId makeFocusScope() {
        const auto type = findBlueprint<T>();
        if (type == Blueprints::InvalidId)
            throw Exception(tag, "Scope type '{}' is not registered", typeKey<T>());
        return makeFocusScope(type);
    }

    FocusScopeId makeFocusScope(BlueprintId type) {
        if (type != Blueprints::InvalidId)
            run_ctx.blueprints.require(type);
        if (focusScope != InvalidFocusScopeId) {
            if (run_ctx.focusScopes.require(focusScope).type != type)
                throw Exception(tag, "Cannot change the type of an existing focus scope");
            return focusScope;
        }
        const auto previousSelections = run_ctx.activeScopes;
        focusScope = run_ctx.createFocusScope(id, type);
        if (!parent)
            run_ctx.rootScope = focusScope;
        try {
            // Inserting a typed ancestor must also update existing selections.
            for (auto selected : previousSelections)
                run_ctx.activateFocus(selected);
            run_ctx.activateFocusIfTyped(focusScope);
            run_ctx.rebuildFocus();
        } catch (...) {
            run_ctx.focusScopes.destroy(focusScope);
            focusScope = InvalidFocusScopeId;
            run_ctx.activeScopes = previousSelections;
            run_ctx.rebuildFocus();
            throw;
        }
        return focusScope;
    }

    template<class T>
    Focus<T> focus(std::string_view role = typeKey<T>()) {
        static_assert(!std::is_same_v<T, Node>, "Use id() for low-level node access");
        return Focus<T>(run_ctx, run_ctx.getOrCreateRole(role), nearestFocusScope());
    }

    // Assign a role in the nearest scope and select that scope if it is typed.
    void setFocus(std::string_view role, ObjectId target) {
        if (target != InvalidObjectId)
            run_ctx.objects.require(target);
        const auto scope = nearestFocusScope();
        run_ctx.setFocus(scope, run_ctx.getOrCreateRole(role), target);
        run_ctx.activateFocusIfTyped(scope);
    }

    Context& requireContext() noexcept { return run_ctx; }
    const Blueprint* getBlueprint() const noexcept { return run_ctx.blueprints.get(bp); }
    BlueprintId getBlueprintId() const noexcept { return bp; }
    Node* getParent() noexcept { return parent; }
    const Node* getParent() const noexcept { return parent; }
    NodeKind getKind() const noexcept { return kind; }
    ObjectId getId() const noexcept { return id; }
    std::string_view name() const {
        return run_ctx.objects.require(id).name;
    }

    BlueprintId findBlueprint(std::string_view name) const {
        return run_ctx.blueprints.resolve(name);
    }

    template<typename T>
    BlueprintId findBlueprint() const {
        return run_ctx.blueprints.find(typeKey<T>());
    }

    template<typename API>
    void addImpls() {
        const auto api = findBlueprint<API>();
        if (api == Blueprints::InvalidId)
            throw Exception(tag, "API '{}' is not registered", typeKey<API>());
        // Factories can register additional types; do not retain registry references.
        std::vector<std::string> names;
        for (BlueprintId id = 0; id < run_ctx.blueprints.size(); ++id)
            if (id != api && run_ctx.blueprints.isA(id, api) && run_ctx.blueprints.require(id).meta.create)
                names.push_back(run_ctx.blueprints.require(id).name);
        for (const auto& name : names)
            add(name, name);
    }

    template<typename T>
    std::vector<T*> directCollect() const {
        std::vector<T*> out;

        BlueprintId apiId = findBlueprint<T>();
        if (apiId == Blueprints::InvalidId)
            return out;

        for (const auto& child : children_) {
            if (!child->isImplement(apiId))
                continue;

            if (void* object = child->castObject(apiId))
                out.push_back(static_cast<T*>(object));
        }

        return out;
    }

    // возвращает объекты интерфейса <T> из текущего узла и его потомков
    template<typename API>
    std::vector<API*> folderCollect() const {
        BlueprintId apiId = findBlueprint<API>();

        if (apiId == Blueprints::InvalidId)
            return {};

        std::vector<ObjectId> ids;
        collectInto(apiId, ids);

        std::vector<API*> out;
        out.reserve(ids.size());

        for (ObjectId id : ids) {
            auto* node = run_ctx.objects.require(id).node;

            if (void* object = node->castObject(apiId))
                out.push_back(static_cast<API*>(object));
        }

        return out;
    }

    // возвращает все объекты интерфейса <T> существующие в дереве (начиная с root)
    template<typename T>
    std::vector<T*> globalCollect() const {
        return rootNode().folderCollect<T>();
    }

    // объекты API на этаже: потомки родителя (соседи и их дети), не через Context
    template<typename API>
    std::vector<API*> collect() const {
        const Node& floor = parent ? *parent : *this;
        return floor.folderCollect<API>();
    }

    template<typename T>
    Children<T> children() const {
        return Children<T>(directCollect<T>());
    }

    Node& addFolder(std::string_view name) {
        return createNode(name);
    }

    template<typename T>
    Ref<T> add(std::string_view instanceName = DefaultInstanceName) {
        noteAdd<T>();
        return Ref<T>(createComponent(typeKey<T>(), instanceName, nullptr));
    }

    template<typename T, typename D>
    requires CreationDescriptor<T, D>
    Ref<T> add(std::string_view instanceName, const D& desc) {
        noteAdd<T>();
        return Ref<T>(createComponent(typeKey<T>(), instanceName, &desc, typeKey<D>()));
    }

    void add(std::string_view type, std::string_view instanceName) {
        addNode(type, instanceName);
    }

    Node& addNode(std::string_view type, std::string_view instanceName) {
        return createComponent(type, instanceName, nullptr);
    }

private:
    Node& createComponent(std::string_view parent, std::string_view instanceName, const void* desc,
                          std::string_view descriptor = {}) {
        instanceName = canonicalInstance(instanceName);

        const auto api = findBlueprint(parent);
        if (api == Blueprints::InvalidId)
            throw Exception(tag, "Blueprint '{}' not found", parent);
        const auto* owner = run_ctx.blueprints.get(implementation());
        const auto blueprint = run_ctx.blueprints.resolveImplementation(
            api, owner ? owner->namespaceName() : std::string_view{}, descriptor);
        const auto create = run_ctx.blueprints.require(blueprint).meta.create;

        for (const auto& child : children_) {
            if (sameInstance(child->name(), instanceName) && child->bp == blueprint) {
                Logger::warning(tag, "object '{}' with instance '{}' already exists", parent, instanceName);
                return *child;
            }
        }

        Node& child = createNode(instanceName, blueprint);
        child.kind = NodeKind::Component;
        try {
            child.object = create(child, desc);
        } catch (...) {
            std::erase_if(children_, [&child](const auto& entry) { return entry.get() == &child; });
            throw;
        }

        if (unnamedInstance(instanceName))
            Logger::info(tag, "added '{}'", run_ctx.blueprints.require(blueprint).shortName());
        else
            Logger::info(tag, "added '{}:{}'", run_ctx.blueprints.require(blueprint).shortName(), instanceName);

        return child;
    }

public:
    template<typename API, typename Impl>
    void use(std::string_view instanceName = DefaultInstanceName) {
        useImplementation<API, Impl>(instanceName, nullptr);
    }

    template<typename API, typename Impl, typename D>
    requires CreationDescriptor<Impl, D>
    void use(std::string_view instanceName, const D& desc) {
        useImplementation<API, Impl>(instanceName, &desc);
    }

    template<typename API>
    void use(std::string_view implName) {
        useSlot<API>(implName, nullptr);
    }

private:
    template<typename API, typename Impl>
    void useImplementation(std::string_view instanceName, const void* desc) {
        noteUseImpl<API, Impl>();
        auto found = find<API>(instanceName);
        if (!found.node())
            throw Exception(tag, "slot '{}' with instance '{}' not found", typeName<API>(), instanceName);
        found.node()->template useSlot<API>(typeKey<Impl>(), desc);
    }

    template<typename API>
    void useSlot(std::string_view implName, const void* desc) {
        if (kind != NodeKind::Slot || bp == Blueprints::InvalidId)
            throw Exception(tag, "use() requires a slot node");

        const BlueprintId apiId = bp;
        const BlueprintId implId = findBlueprint(implName);
        if (implId == Blueprints::InvalidId)
            throw Exception(tag, "unknown implementation '{}'", implName);

        if (!run_ctx.blueprints.isA(implId, apiId))
            throw Exception(tag, "implementation '{}' does not provide '{}'", implName, typeKey<API>());
        const auto create = run_ctx.blueprints.require(implId).meta.create;
        if (!create)
            throw Exception(tag, "blueprint '{}' has no create callback", implName);

        stopServices();
        children_.clear();
        destroyObject();
        implBp = implId;
        try {
            object = create(*this, desc);
            if (!castObject(apiId))
                throw Exception(tag, "implementation '{}' has no conversion to '{}'", implName, typeKey<API>());
        } catch (...) {
            children_.clear();
            destroyObject();
            throw;
        }

        Logger::info(tag, "> use '{}' = '{}'", typeName<API>(), implName);

        configured = false;
        configureBranch();
        reconfigureFloor();
    }

public:
    template<typename API>
    Slot<API> slot(std::string_view instanceName = DefaultInstanceName) {
        noteUse<API>();
        instanceName = canonicalInstance(instanceName);

        const BlueprintId apiId = findBlueprint<API>();
        if (apiId == Blueprints::InvalidId)
            throw Exception(tag, "unknown API '{}'", typeName<API>());

        const auto blueprint = apiId;

        for (const auto& child : children_) {
            if (child->kind == NodeKind::Slot &&
                sameInstance(child->name(), instanceName) &&
                child->bp == apiId)
                return Slot<API>(*child);
        }

        Node& child = createNode(instanceName);
        child.bp = blueprint;
        child.kind = NodeKind::Slot;

        Logger::info(tag, "+ slot '{}'", typeName<API>());
        return Slot<API>(child);
    }

    // ищет слот/импл на этаже: дети текущего узла, затем дети предков вверх. Не через Context.
    template<typename API>
    Slot<API> find(std::string_view instanceName = DefaultInstanceName) {
        noteRequire<API>();
        instanceName = canonicalInstance(instanceName);

        BlueprintId apiId = findBlueprint<API>();

        if (apiId == Blueprints::InvalidId)
            return {};

        for (const auto& child : children_) {
            if (!sameInstance(child->name(), instanceName))
                continue;

            if (child->isImplement(apiId))
                return Slot<API>(*child);
        }

        return parent ? parent->find<API>(instanceName) : Slot<API>{};
    }

    // ищет слот/импл на этаже. Пустой слот (нода есть, object == nullptr) — исключение.
    template<typename T>
    Ref<T> require(std::string_view instanceName = DefaultInstanceName) {
        noteRequire<T>();

        auto found = find<T>(instanceName);
        if (!found.node())
            throw Lattice::Exception(tag, "Object '{}' with instance '{}' not found", typeName<T>(), instanceName);

        if (!found.get())
            throw Lattice::Exception(tag, "Slot '{}' with instance '{}' is empty", typeName<T>(), instanceName);

        return Ref<T>(*found.node());
    }

    // Resolve the direct owner, independent of its instance name or siblings.
    template<typename T>
    Ref<T> requireParent() {
        noteRequire<T>();
        T* ptr = parent ? parent->get<T>() : nullptr;
        if (!ptr)
            throw Exception(tag, "Parent does not provide '{}'", typeKey<T>());
        return Ref<T>(*parent);
    }

    Node& require(std::string_view type, std::string_view instanceName = DefaultInstanceName) {
        instanceName = canonicalInstance(instanceName);
        BlueprintId blueprintId = findBlueprint(type);
        if (blueprintId == Blueprints::InvalidId)
            throw Lattice::Exception(tag, "Blueprint '{}' not found", type);

        for (const auto& child : children_) {
            if (sameInstance(child->name(), instanceName) && child->bp == blueprintId)
                return *child.get();
        }

        if (parent)
            return parent->require(type, instanceName);

        throw Lattice::Exception(tag, "Object '{}.{}' not found", type, instanceName);
    }

    bool isUnder(ObjectId ancestorId) const noexcept {
        const Node* node = this;
        while (node) {
            if (node->id == ancestorId)
                return true;
            node = node->parent;
        }
        return false;
    }

    template<typename T>
    Mount<T> mount(std::string_view role = typeKey<T>()) {
        noteRequire<T>();
        const BlueprintId api = findBlueprint<T>();
        if (api == Blueprints::InvalidId)
            throw Exception(tag, "unknown API '{}' for mount", typeKey<T>());

        for (const auto& child : children_) {
            if (child->kind == NodeKind::Mount &&
                sameInstance(child->name(), DefaultInstanceName) &&
                child->isImplement(api))
                return Mount<T>(*child);
        }

        auto selected = focus<T>(role);
        if (!selected.get())
            throw Exception(tag, "Focus role '{}' does not provide '{}' for mount", role, typeKey<T>());

        Node& targetNode = *run_ctx.objects.require(selected.id()).node;
        const BlueprintId impl = targetNode.implementation();
        void* object = targetNode.getObject();
        if (impl == Blueprints::InvalidId || !object)
            throw Exception(tag, "Focus role '{}' does not provide '{}' for mount", role, typeKey<T>());

        Node& mountNode = createNode(DefaultInstanceName, impl);
        mountNode.kind = NodeKind::Mount;
        mountNode.object = object;
        mountNode.configured = true;
        return Mount<T>(mountNode);
    }

    ObjectId resolvePath(std::string_view path, ObjectId from) const {
        ObjectId current = from;

        size_t begin = 0;
        while (begin < path.size()) {
            size_t end = path.find('/', begin);
            if (end == std::string_view::npos)
                end = path.size();

            std::string_view name = path.substr(begin, end - begin);

            if (!name.empty()) {
                current = run_ctx.objects.find(ObjectKey{std::string(name), current});
                if (current == InvalidObjectId)
                    return InvalidObjectId;
            }

            begin = end + 1;
        }

        return current;
    }

    // вызывает метод configure() у всех компонентов ветки
    void configureBranch() {
        if (!configured)
            configure();

        for (auto& child : children_)
            child->configureBranch();
    }

    // удаляет компонент <T> из ветки
    template<typename API>
    void remove(std::string_view instanceName = DefaultInstanceName) {
        instanceName = canonicalInstance(instanceName);
        BlueprintId apiId = findBlueprint<API>();

        if (apiId == Blueprints::InvalidId)
            return;

        auto child = std::find_if(
            children_.begin(),
            children_.end(),
            [&](const std::unique_ptr<Node>& node) {
                return sameInstance(node->name(), instanceName) &&
                    node->isImplement(apiId);
            }
        );

        if (child != children_.end())
            children_.erase(child);
    }

    // останавливает все сервисы
    void stopServices() {
        if (kind != NodeKind::Mount) {
            if (auto* service = get<ServiceAPI>())
                service->stop();
        }
        for (auto& child : children_)
            child->stopServices();
    }

    ~Node() {
        if (kind == NodeKind::Binding)
            run_ctx.bindings.unbind(id);

        if (kind != NodeKind::Mount) {
            if (auto* service = get<ServiceAPI>())
                service->stop();
        }
        children_.clear();
        if (kind == NodeKind::Component || kind == NodeKind::Slot)
            destroyObject();

        run_ctx.removeFocusObject(id);
        run_ctx.objects.destroy(id);
    }

    void dumpTree(std::string_view path = "") const {
        ObjectId startId = resolvePath(path, id);
        if (startId != InvalidObjectId) {
            Node* startNode = run_ctx.objects.require(startId).node;
            Logger::Tree tree(run_ctx.objects.require(startId).name);
            startNode->appendTree(tree, 0, 7);
            tree.print();
        } else {
            Logger::warning(tag, "not found path at '{}'", path);
        }
    }

    void* castObject(BlueprintId api) const {
        return run_ctx.blueprints.cast(implementation(), api, object);
    }

    template<typename T>
    T* get() const {
        return static_cast<T*>(castObject(findBlueprint<T>()));
    }

    void* getObject() const noexcept {
        return object;
    }

    std::vector<ObjectId> collectTree() const {
        std::vector<ObjectId> ids;

        std::function<void(const Node&)> collect = [&](const Node& node) {
            for (const auto& child : node.children_) {
                ids.push_back(child->id);
                collect(*child);
            }
        };

        collect(*this);
        return ids;
    }
    
    template<typename T>
    ObjectId bind(std::string_view name, T* ptr, double min = 0, double max = 0, bool hasRange = false) {
        Node& child = bindingChild(name);
        child.object = &run_ctx.bindings.bind(child.id, ptr, min, max, hasRange);
        run_ctx.setFocus(nearestFocusScope(), run_ctx.getOrCreateRole(name), child.id);
        return child.id;
    }

    template<typename T, typename F>
    requires std::invocable<F&, T>
    ObjectId bind(std::string_view name, T* ptr, F&& onChange, double min = 0, double max = 0, bool hasRange = false) {
        Node& child = bindingChild(name);
        child.object = &run_ctx.bindings.bind(child.id, ptr, std::forward<F>(onChange), min, max, hasRange);
        run_ctx.setFocus(nearestFocusScope(), run_ctx.getOrCreateRole(name), child.id);
        return child.id;
    }

    ObjectId on(std::string_view name, std::function<void()> handler) {
        Node& child = bindingChild(name);
        child.object = &run_ctx.bindings.on(child.id, std::move(handler));
        run_ctx.setFocus(nearestFocusScope(), run_ctx.getOrCreateRole(name), child.id);
        return child.id;
    }

    std::string stringPath() const {
        std::string result;
        Path path{id, run_ctx.objects};

        for (ObjectId current : path.ids()) {
            const auto& entry = run_ctx.objects.require(current);
            const Node& node = *entry.node;

            if (!result.empty())
                result += '/';

            if (node.bp == Blueprints::InvalidId) {
                result += entry.name;
                continue;
            }

            if (entry.name.empty() || entry.name == "default" || entry.name == "Root")
                result += std::string(run_ctx.blueprints.require(node.bp).shortName());
            else
                result += std::format("{}:{}", run_ctx.blueprints.require(node.bp).shortName(), entry.name);
        }

        return result;
    }
};

inline Node* handleNode(Context* ctx, ObjectId id) {
    if (!ctx || id == InvalidObjectId)
        return nullptr;
    const auto* object = ctx->objects.get(id);
    return object ? object->node : nullptr;
}

template<typename T>
Slot<T>::Slot(Node& node) : ctx(&node.requireContext()), id(node.getId()) {}

template<typename T>
Node* Slot<T>::node() const {
    return handleNode(ctx, id);
}

template<typename T>
T* Slot<T>::get() const {
    Node* n = node();
    return n ? n->template get<T>() : nullptr;
}

template<typename T>
bool Slot<T>::exists() const {
    return get() != nullptr;
}

template<typename T>
void Slot<T>::use(std::string_view implName) {
    Node* n = node();
    if (!n)
        throw Exception("Node", "use() on empty slot handle");
    n->template use<T>(implName);
}

template<typename T>
Slot<T>& Slot<T>::focus(std::string_view role) {
    Node* n = node();
    if (!n)
        throw Exception("Slot", "focus() on empty handle");
    n->setFocus(role, id);
    if (role == typeKey<T>() && typeName<T>() != typeKey<T>())
        n->setFocus(typeName<T>(), id);
    return *this;
}

template<typename T>
Ref<T>::Ref(Node& node) : ctx(&node.requireContext()), id(node.getId()) {}

template<typename T>
Node* Ref<T>::node() const {
    return handleNode(ctx, id);
}

template<typename T>
T* Ref<T>::getPtr() const {
    Node* n = node();
    if (!n)
        return nullptr;
    if constexpr (std::is_same_v<T, Node>)
        return n;
    else
        return n->template get<T>();
}

template<typename T>
Ref<T>& Ref<T>::focus(std::string_view role) {
    Node* n = node();
    if (!n)
        throw Exception("Ref", "focus() on empty handle");
    n->setFocus(role, id);
    if (role == typeKey<T>() && typeName<T>() != typeKey<T>())
        n->setFocus(typeName<T>(), id);
    return *this;
}

template<typename T>
Node& Ref<T>::branch() {
    Node* n = node();
    if (!n)
        throw Exception("Ref", "branch() on empty handle");
    return *n;
}

template<typename T>
const Node& Ref<T>::branch() const {
    Node* n = node();
    if (!n)
        throw Exception("Ref", "branch() on empty handle");
    return *n;
}

template<typename T>
template<typename U>
Ref<U> Ref<T>::add(std::string_view instanceName) {
    return branch().template add<U>(instanceName);
}

template<typename T>
template<typename U, typename D>
Ref<U> Ref<T>::add(std::string_view instanceName, const D& desc) {
    return branch().template add<U>(instanceName, desc);
}

template<typename T>
template<typename U>
Slot<U> Ref<T>::slot(std::string_view instanceName) {
    return branch().template slot<U>(instanceName);
}

template<typename T>
template<typename API, typename Impl>
void Ref<T>::use(std::string_view instanceName) {
    branch().template use<API, Impl>(instanceName);
}

template<typename T>
template<typename API>
void Ref<T>::use(std::string_view implName) {
    branch().template use<API>(implName);
}

template<typename T>
template<typename U>
Slot<U> Ref<T>::find(std::string_view instanceName) {
    return branch().template find<U>(instanceName);
}

template<typename T>
template<typename U>
Ref<U> Ref<T>::require(std::string_view instanceName) {
    return branch().template require<U>(instanceName);
}

template<typename T>
template<typename U>
ObjectId Ref<T>::bind(std::string_view name, U* ptr, double min, double max, bool hasRange) {
    return branch().bind(name, ptr, min, max, hasRange);
}

template<typename T>
template<typename U, typename F>
ObjectId Ref<T>::bind(std::string_view name, U* ptr, F&& onChange, double min, double max, bool hasRange) {
    return branch().bind(name, ptr, std::forward<F>(onChange), min, max, hasRange);
}

template<typename T>
ObjectId Ref<T>::on(std::string_view name, std::function<void()> handler) {
    return branch().on(name, std::move(handler));
}

template<typename T>
template<typename U>
void Ref<T>::remove(std::string_view instanceName) {
    branch().template remove<U>(instanceName);
}

template<typename T>
template<typename U>
Children<U> Ref<T>::children() const {
    return branch().template children<U>();
}

} // namespace Lattice
