#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <utility>

#include <Lattice/Kernel/ServiceAPI.hpp>
#include <Lattice/Kernel/Requirements.hpp>
#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/RefSlot.hpp>
#include <Lattice/Kernel/Objects.hpp>
#include <Lattice/Kernel/Settings.hpp>
#include <Lattice/Tools/LogStyle.hpp>
#include <Lattice/Tools/Logger.hpp>
#include <Lattice/Tools/LogTree.hpp>
#include <Lattice/Kernel/RuntimeContext.hpp>


namespace Lattice {

class Path {
public:
    Path() = default;

    explicit Path(ObjectId id, Objects& objectBlueprints) {
        while (Objects::valid(id)) {
            ids_.push_back(id);
            id = objectBlueprints.require(id).parent;
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

template<typename T>
concept HasConfigure = requires(T& obj, Node& branch) {
    obj.configure(branch);
};

class Node {
    static constexpr std::string_view tag = "Node";

    RuntimeContext& run_ctx;

    ObjectId id = 0;
    Node* parent = nullptr;
    Node* bp     = nullptr;
    void* object = nullptr; // для чертежей meta

    std::vector<std::unique_ptr<Node>> children_;

    Node* makeChild(std::string_view name, Node* blueprint = nullptr) {
        auto node = std::make_unique<Node>(run_ctx, this);
        Node* raw = node.get();
        raw->id = run_ctx.objects.create(name, id, raw);
        raw->bp = blueprint;
        children_.push_back(std::move(node));
        return raw;
    }

    Meta* getMeta() const noexcept {
        return bp ? static_cast<Meta*>(bp->object) : nullptr;
    }

    std::string_view name() const {
        return run_ctx.objects[id].name;
    }

    template<typename T>
    bool isType() const {
        return bp && bp->name() == typeName<T>();
    }

    bool isImplement(ObjectId apiId) const noexcept {
        return bp && bp->isUnder(apiId);
    }

    void appendTree(Logger::Tree& tree, size_t depth, ObjectId highlighted) const {
        for (const auto& child : children_) {
            const std::string type = child->bp
                ? std::string(child->bp->name())
                : std::string(child->name());
            const std::string name = std::string(child->name());

            const std::string mark =
                child->bp      ? "(O)" :
                child->object  ? "(B)" :
                                 "(F)" ;
            
            std::string line;

            if (mark == "(F)")
                line = std::format("{} <m>(F)</>", name);
            else if (mark == "(B)")
                line = std::format("{} <c>(B)</>", name);
            else
                line = std::format("{}<gr>:{}</> <g>(O)</>", child->bp->name(), name);

            if (child->id == highlighted)
                line = std::format("<b><r>{} 🡸<//>", line);

            line += std::format(" <gr>#{}</>", child->id);
            tree.node(line, depth);
            child->appendTree(tree, depth + 1, highlighted);
        }
    }

    void collectInto(ObjectId apiId, std::vector<ObjectId>& out) const {
        if (isImplement(apiId))
            out.push_back(id);

        for (const auto& child : children_)
            child->collectInto(apiId, out);
    }

    const Node* rootNode() const {
        const Node* n = this;
        while (n->parent)
            n = n->parent;
        return n;
    }

    Node* rootNode() {
        Node* n = this;
        while (n->parent)
            n = n->parent;
        return n;
    }

    ObjectId findRecursive(std::string_view name, ObjectId from) const {
        ObjectId id = run_ctx.objects.find(name, from);

        if (Objects::valid(id))
            return id;

        const auto* node = static_cast<const Node*>(run_ctx.objects[from].object);

        for (const auto& child : node->children_) {
            id = findRecursive(name, child->id);
            if (Objects::valid(id))
                return id;
        }

        return InvalidObjectId;
    }

public:
    Node(RuntimeContext& run_ctx, Node* parent = nullptr)
        : run_ctx(run_ctx), parent(parent) {
            if (!parent) {
                id = run_ctx.objects.create("Root", InvalidObjectId, this);
            }
    }

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator=(Node&&) = delete;

    RuntimeContext& get_ctx() noexcept { return run_ctx; }
    Node* getBlueprint() noexcept { return bp; }

    template<typename T>
    void blueprint() {
        const std::string name = std::string(typeName<T>());

        if (run_ctx.objects.has(name, id))
            throw Exception(tag, "blueprint '{}' already registered", name);

        auto meta = std::make_unique<Meta>();

        if constexpr (std::is_constructible_v<T, Node&> || std::is_default_constructible_v<T>) {
            meta->create = [](Node& ctx) -> void* {
                if constexpr (std::is_constructible_v<T, Node&>)
                    return new T(ctx);
                else
                    return new T();
            };

            meta->destroy = [](Node& ctx) {
                delete static_cast<T*>(ctx.getObject());
            };

            if constexpr (HasConfigure<T>)
                meta->configure = [](Node& ctx) {
                    static_cast<T*>(ctx.getObject())->configure(ctx);
                };
        }

        Meta* metaPtr = meta.get();
        run_ctx.metas.push_back(std::move(meta));

        Node* child = makeChild(name);
        child->object = metaPtr;

        Logger::info(tag, "+ blueprint {}", name);
    }

    template<typename Impl, typename API>
    void blueprint(std::string_view blueprintsPath = DefaultBlueprintsPath) {
        ObjectId apiId = findBlueprint<API>(blueprintsPath);

        if (!Objects::valid(apiId))
            throw Exception(tag, "API '{}' is not registered", typeName<API>());

        Node* api = static_cast<Node*>(run_ctx.objects.require(apiId).object);
        api->blueprint<Impl>();
    }

    ObjectId primitive(std::string_view name) {
        Node* child = makeChild(name);
        return child->id;
    }

    ObjectId findBlueprint(std::string_view name, std::string_view blueprintsPath = DefaultBlueprintsPath) const {
        ObjectId folderId = resolvePath(blueprintsPath, rootNode()->id);
        if (!Objects::valid(folderId))
            return InvalidObjectId;

        return findRecursive(name, folderId);
    }

    template<typename T>
    ObjectId findBlueprint(std::string_view blueprintsPath = DefaultBlueprintsPath) const {
        return findBlueprint(typeName<T>(), blueprintsPath);
    }

    // создает и возвращает объекты интерфейса <T> найденные в глобальном blueprints
    template<typename API>
    void addImpls(std::string_view blueprintsPath = DefaultBlueprintsPath) {
        ObjectId apiId = findBlueprint<API>(blueprintsPath);

        if (!Objects::valid(apiId))
            throw Exception(tag, "API '{}' is not registered", typeName<API>());

        Node* api = static_cast<Node*>(run_ctx.objects.require(apiId).object);

        for (const auto& child : api->children_)
            add(child->name(), child->name(), blueprintsPath);
    }

    template<typename T>
    std::vector<T*> directCollect() const {
        std::vector<T*> out;

        for (const auto& child : children_) {
            if (!child->isType<T>())
                continue;

            if (void* object = child->getObject())
                out.push_back(static_cast<T*>(object));
        }

        return out;
    }

    // возвращает объекты интерфейса <T> из текущего узла и его потомков
    template<typename API>
    std::vector<API*> folderCollect() const {
        ObjectId apiId = findBlueprint<API>();

        if (!Objects::valid(apiId))
            return {};

        std::vector<ObjectId> ids;
        collectInto(apiId, ids);

        std::vector<API*> out;
        out.reserve(ids.size());

        for (ObjectId id : ids) {
            const auto& entry = run_ctx.objects.require(id);
            auto* node = static_cast<Node*>(entry.object);

            if (void* object = node->getObject())
                out.push_back(static_cast<API*>(object));
        }

        return out;
    }

    // возвращает все объекты интерфейса <T> существующие в дереве (начиная с root)
    template<typename T>
    std::vector<T*> globalCollect() const {
        return rootNode()->folderCollect<T>();
    }

    Node& addFolder(std::string_view name) {
        return *makeChild(name);
    }
    
    template<typename T>
    void add(std::string_view instanceName = "default", std::string_view blueprintsPath = DefaultBlueprintsPath) {
        noteAdd<T>();
        add(typeName<T>(), instanceName, blueprintsPath);
    }

    void add(std::string_view parent, std::string_view instanceName = "default", std::string_view blueprintsPath = DefaultBlueprintsPath) {
        ObjectId folderId = resolvePath(blueprintsPath, rootNode()->id);
        if (!Objects::valid(folderId))
            throw Exception(tag, "blueprint folder '{}' not found", blueprintsPath);

        ObjectId blueprintId = findBlueprint(parent, blueprintsPath);
        if (!Objects::valid(blueprintId))
            throw Exception(tag, "blueprint '{}' not found in '{}'", parent, blueprintsPath);

        Node* blueprint = static_cast<Node*>(run_ctx.objects.require(blueprintId).object);

        for (const auto& child : children_) {
            if (child->name() == instanceName && child->bp == blueprint) {
                Logger::warning(tag, "object '{}' with instance '{}' already exists", parent, instanceName);
                return;
            }
        }

        Node* child = makeChild(instanceName, blueprint);
        child->object = static_cast<Meta*>(blueprint->object)->create(*child);

        Logger::info(tag, "+ {}", instanceName);
    }

    template<typename API, typename Impl>
    Slot<API> use(std::string_view instanceName = "default", std::string_view blueprintsPath = DefaultBlueprintsPath) {
        noteUseImpl<API, Impl>();
        return use<API>(typeName<Impl>(), instanceName, blueprintsPath);
    }

    template<typename API>
    Slot<API> use(std::string_view implName, std::string_view instanceName = "default", std::string_view blueprintsPath = DefaultBlueprintsPath) {
        noteUse<API>();

        ObjectId folderId = resolvePath(blueprintsPath, rootNode()->id);
        if (!Objects::valid(folderId))
            throw Exception(tag, "blueprint folder '{}' not found", blueprintsPath);

        const ObjectId apiId = findBlueprint<API>(blueprintsPath);
        const ObjectId implId = findBlueprint(implName, blueprintsPath);

        if (!Objects::valid(apiId))
            throw Exception(tag, "unknown API '{}' in '{}'", typeName<API>(), blueprintsPath);

        if (!Objects::valid(implId))
            throw Exception(tag, "unknown implementation '{}' in '{}'", implName, blueprintsPath);

        Node* blueprint = static_cast<Node*>(run_ctx.objects.require(implId).object);

        if (!blueprint->isUnder(apiId))
            throw Exception(tag, "implementation '{}' does not provide '{}'", implName, typeName<API>());

        ObjectId objectId = run_ctx.objects.find(instanceName, id);
        Node* child = nullptr;

        for (auto& node : children_) {
            if (node->name() == instanceName && node->bp && node->bp->isUnder(apiId)) {
                child = node.get();
                break;
            }
        }

        if (child) {
            if (child->object) {
                Meta* meta = static_cast<Meta*>(child->bp->object);
                if (meta && meta->destroy)
                    meta->destroy(*child);
            }

            child->children_.clear();
            child->object = nullptr;
        } else {
            child = makeChild(instanceName);
            Logger::info(tag, "+ interface '{}'", typeName<API>());
        }

        child->bp = blueprint;

        Meta* meta = static_cast<Meta*>(blueprint->object);
        if (!meta || !meta->create)
            throw Exception(tag, "blueprint '{}' has no create callback", implName);

        child->object = meta->create(*child);

        Logger::info(tag, "> use '{}' = '{}'", typeName<API>(), implName);
        return Slot<API>(child);
    }

    // ищет компонент <T> в текущем узле и родительских
    // если компонент не найден возвращает nullptr
    template<typename API>
    Slot<API> find(std::string_view instanceName = "default", std::string_view blueprintsPath = DefaultBlueprintsPath) {
        noteRequire<API>();

        ObjectId apiId = findBlueprint<API>(blueprintsPath);

        if (!Objects::valid(apiId))
            return {};

        for (const auto& child : children_) {
            if (child->name() != instanceName)
                continue;

            if (child->bp && child->bp->isUnder(apiId))
                return Slot<API>(child.get());
        }

        return parent ? parent->find<API>(instanceName, blueprintsPath) : Slot<API>{};
    }

    // ищет компонент <T> в текущем узле и родительских
    // если компонент не найден кидает исключение
    template<typename T>
    Ref<T> require(std::string_view instanceName = "default", std::string_view blueprintsPath = DefaultBlueprintsPath) {
        noteRequire<T>();

        if (auto slot = find<T>(instanceName, blueprintsPath); slot.exists())
            return Ref<T>(slot.get());

        throw Lattice::Exception(tag, "Object '{}' with instance '{}' not found", typeName<T>(), instanceName);
    }

    Node& require(std::string_view type, std::string_view instanceName = "default", std::string_view blueprintsPath = DefaultBlueprintsPath) {
        ObjectId blueprintId = findBlueprint(type, blueprintsPath);
        if (!Objects::valid(blueprintId))
            throw Lattice::Exception(tag, "Blueprint '{}' not found", type);

        for (const auto& child : children_) {
            if (child->name() == instanceName && child->bp && child->bp->id == blueprintId)
                return *child.get();
        }

        if (parent)
            return parent->require(type, instanceName, blueprintsPath);

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

    ObjectId resolvePath(std::string_view path, ObjectId from) const {
        ObjectId current = from;

        size_t begin = 0;
        while (begin < path.size()) {
            size_t end = path.find('/', begin);
            if (end == std::string_view::npos)
                end = path.size();

            std::string_view name = path.substr(begin, end - begin);

            if (!name.empty()) {
                current = run_ctx.objects.find(name, current);
                if (!Objects::valid(current))
                    return InvalidObjectId;
            }

            begin = end + 1;
        }

        return current;
    }

    // вызывает метод configure() у всех компонентов ветки
    void configureAll() {
        if (bp && object) {
            Meta* meta = static_cast<Meta*>(bp->object);

            if (meta && meta->configure) {
                Logger::info(tag, "Configuring '{}'", name());
                meta->configure(*this);
            } else {
                Logger::warning(tag, "Object '{}' has no configure callback", name());
            }
        }

        for (auto& child : children_)
            child->configureAll();
    }

    // удаляет компонент <T> из ветки
    template<typename API>
    void remove(std::string_view instanceName = "default", std::string_view blueprintsPath = DefaultBlueprintsPath) {
        ObjectId apiId = findBlueprint<API>(blueprintsPath);

        if (!Objects::valid(apiId))
            return;

        auto child = std::find_if(
            children_.begin(),
            children_.end(),
            [&](const std::unique_ptr<Node>& node) {
                return node->name() == instanceName &&
                    node->bp &&
                    node->bp->isUnder(apiId);
            }
        );

        if (child != children_.end())
            children_.erase(child);
    }

    // останавливает все сервисы
    void stopServices(std::string_view blueprintsPath = DefaultBlueprintsPath) {
        ObjectId folderId = resolvePath(blueprintsPath, rootNode()->id);
        ObjectId serviceId = run_ctx.objects.find(typeName<ServiceAPI>(), folderId);

        if (Objects::valid(serviceId) && bp && bp->isUnder(serviceId) && object) {
            auto* service = static_cast<ServiceAPI*>(object);
            service->stop();
        }

        for (auto& child : children_)
            child->stopServices(blueprintsPath);
    }

    ~Node() {
        if (bp && object) {
            Meta* meta = static_cast<Meta*>(bp->object);

            if (meta && meta->destroy)
                meta->destroy(*this);
        }

        children_.clear();

        if (id)
            run_ctx.objects.destroy(id);
    }

    Path path() const {
        Path path;

        const Node* node = this;
        while (node && node->parent) {
            path.push(node->id);
            node = node->parent;
        }

        return path;
    }

    void dumpTree(std::string_view path = "") const {
        ObjectId startId = resolvePath(path, id);
        if (Objects::valid(startId)) {
            Node* startNode = static_cast<Node*>(run_ctx.objects[startId].object);
            Logger::Tree tree(run_ctx.objects[startId].name);
            startNode->appendTree(tree, 0, 7);
            tree.print();
        } else {
            Logger::warning(tag, "not found path at '{}'", path);
        }
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
        Node* child = makeChild(name, static_cast<Node*>(run_ctx.objects.require(run_ctx.primitives.param).object));
        run_ctx.settings.bind(child->id, ptr, min, max, hasRange);
        return child->id;
    }

    template<typename T, typename F>
    requires std::invocable<F&, T>
    ObjectId bind(std::string_view name, T* ptr, F&& onChange, double min = 0, double max = 0, bool hasRange = false) {
        Node* child = makeChild(name, static_cast<Node*>(run_ctx.objects.require(run_ctx.primitives.param).object));
        run_ctx.settings.bind(child->id, ptr, std::forward<F>(onChange), min, max, hasRange);
        return child->id;
    }

    ObjectId on(std::string_view name, std::function<void()> handler) {
        Node* child = makeChild(name, static_cast<Node*>(run_ctx.objects.require(run_ctx.primitives.action).object));
        run_ctx.settings.on(child->id, std::move(handler));
        return child->id;
    }

    ObjectId param(std::string_view name) const {
        ObjectId pid = run_ctx.objects.find(name, id);
        if (!Objects::valid(pid))
            throw Exception(tag, "param '{}' not found", name);
        return pid;
    }

    ObjectId action(std::string_view name) const {
        ObjectId aid = run_ctx.objects.find(name, id);
        if (!Objects::valid(aid))
            throw Exception(tag, "action '{}' not found", name);
        return aid;
    }

    template<typename T>
    T get(std::string_view name) const {
        return run_ctx.settings.get<T>(param(name));
    }

    template<typename T>
    void set(std::string_view name, T value) {
        run_ctx.settings.set(param(name), std::move(value));
    }

    void fire(std::string_view name) const {
        run_ctx.settings.fire(action(name));
    }

    void activate(std::string_view role) {
        run_ctx.context.set(role, id);
    }

    void onActivate(std::string_view role) {
        on("activate", [this, r = std::string(role)] { activate(r); });
        activate(role);
    }

    std::vector<ObjectId> path(ObjectId id) const {
        std::vector<ObjectId> result;

        while (Objects::valid(id)) {
            result.push_back(id);
            id = run_ctx.objects[id].parent;
        }

        std::ranges::reverse(result);
        return result;
    }

    const std::string stringPath(ObjectId id) const {
        std::string result;

        for (ObjectId current : path(id)) {
            const auto& entry = run_ctx.objects[current];
            const auto* node = static_cast<const Node*>(entry.object);

            if (!result.empty())
                result += '/';

            if (entry.name.empty() || entry.name == "default" || entry.name == "Root")
                result += std::format("{}", node->bp->name());
            else
                result += std::format("{}:{}", node->bp->name(), entry.name);
        }

        return result;
    }

    ObjectId getId() {
        return id;
    }
};

template<typename T>
T* Slot<T>::get() const noexcept {
    return node
        ? static_cast<T*>(node->getObject())
        : nullptr;
}

template<typename T>
bool Slot<T>::exists() const noexcept {
    return get() != nullptr;
}
} // namespace Lattice
