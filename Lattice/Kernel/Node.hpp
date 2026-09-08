#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <utility>

#include <Lattice/Kernel/Registry.hpp>
#include <Lattice/Kernel/ServiceAPI.hpp>
#include <Lattice/Kernel/Requirements.hpp>
#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/RefSlot.hpp>
#include <Lattice/Kernel/ObjectRegistry.hpp>
#include <Lattice/Kernel/Settings.hpp>
#include <Lattice/Tools/LogStyle.hpp>
#include <Lattice/Tools/Logger.hpp>
#include <Lattice/Tools/LogTree.hpp>
#include "Lattice/Kernel/Kernel.hpp"


namespace Lattice {

class Registry;

class Node {
    static constexpr std::string_view tag = "Node";

    Kernel& kernel_;
    Node* parent = nullptr;
    ObjectId id = 0;

    std::vector<std::unique_ptr<Node>> children;

    using DestroyFn   = void(*)(void*);
    using ConfigureFn = void(*)(void*, Node&);

    void* instance        = nullptr;
    void* api             = nullptr;
    DestroyFn destroy     = nullptr;
    ConfigureFn configure = nullptr;

    Node* makeChild(std::string_view name, std::string_view type) {
        auto node = std::make_unique<Node>(kernel_, this);
        Node* raw = node.get();
        raw->id = kernel_.objects.create(id, type, name, raw);
        children.push_back(std::move(node));
        return raw;
    }

    void applyRoles(Node* child, const Registry::TypeEntry& entry) {
        child->instance  = entry.create(child);
        child->api       = child->instance; // каст в use/find шаблоном
        child->destroy   = entry.destroy;
        child->configure = entry.configure;

        for (const auto& role : entry.implements)
            kernel_.objects.alias(child->id, id, role, kernel_.objects[child->id].name);
    }

    void appendTree(Logger::Tree& tree, size_t depth, const ObjectId highlighted) const {
        for (const auto& child : children) {
            const Entry& entry = kernel_.objects.require(child->id);
            std::string label = entry.type;
            if (child->id == highlighted)
                label = std::format("<b><r>{} 🡸<//>", label);

            tree.node(std::format("{} <gr>({}) #{}</>", label, entry.name, child->id), depth);
            child->appendTree(tree, depth + 1, highlighted);
        }
    }

    template<typename T>
    void collectInto(std::vector<ObjectId>& out) const {
        if (kernel_.objects.hasRole(id, typeName<T>()))
            out.push_back(id);

        for (const auto& child : children)
            child->collectInto<T>(out);
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

public:
    Node(Kernel& kernel_, Node* parent = nullptr)
        : kernel_(kernel_), parent(parent) {
            if (!parent)
                id = kernel_.objects.create(InvalidObjectId, "Root", "Root", this);
    }

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator=(Node&&) = delete;

    Kernel& kernel() noexcept { return kernel_; }

    // создает и возвращает объекты интерфейса <T> найденные в глобальном registry
    template<typename T>
    void addImpls() {
        for (const auto& implName : kernel_.registry.implementationsOf<T>())
            add<T>(implName, implName);
    }

    template<typename T>
    std::vector<T*> directCollect() const {
        std::vector<T*> out;

        for (const auto& child : children) {
            if (!kernel_.objects.hasRole(child->id, typeName<T>()))
                continue;

            if (void* object = child->getObject())
                out.push_back(static_cast<T*>(object));
        }

        return out;
    }

    // возвращает объекты интерфейса <T> из текущего узла и его потомков
    template<typename T>
    std::vector<T*> folderCollect() const {
        std::vector<ObjectId> ids;
        collectInto<T>(ids);

        std::vector<T*> out;
        out.reserve(ids.size());

        for (ObjectId id : ids) {
            const auto& entry = kernel_.objects.require(id);
            auto* node = static_cast<Node*>(entry.object);

            if (void* object = node->getObject())
                out.push_back(static_cast<T*>(object));
        }

        return out;
    }

    // возвращает все объекты интерфейса <T> существующие в дереве (начиная с root)
    template<typename T>
    std::vector<T*> globalCollect() const {
        return rootNode()->folderCollect<T>();
    }

    Node& addFolder(std::string_view name) {
        Node* raw = makeChild(name, "Folder");
        return *raw;
    }

    template<typename T>
    void add(std::string_view implName, std::string_view instanceName) {
        noteAdd<T>();

        const auto& entry = kernel_.registry.requireImpl<T>(implName);
        Node* child = makeChild(instanceName, implName);
        applyRoles(child, entry);

        Logger::info(tag, "+ {} '{}' ({})", typeName<T>(), instanceName, implName);
    }

    template<typename T, typename Impl>
    void add(std::string_view instanceName = "default") {
        noteAdd<T>();

        const auto& entry = kernel_.registry.requireImpl<T>(typeName<Impl>());
        Node* child = makeChild(instanceName, typeName<Impl>());
        applyRoles(child, entry);

        Logger::info(tag, "+ {} '{}' ({})", typeName<T>(), instanceName, typeName<Impl>());
    }

    template<typename T>
    void add(std::string_view instanceName = "default") {
        noteAdd<T>();

        if (has(typeName<T>(), instanceName)) {
            return;
        }

        const auto& entry = kernel_.registry.require<T>();
        Node* child = makeChild(instanceName, typeName<T>());
        applyRoles(child, entry);

        Logger::info(tag, "+ {}", typeName<T>());
    }

    template<typename API, typename Impl>
    Slot<API> use(std::string_view instanceName = "default") {
        noteUseImpl<API, Impl>();
        return use<API>(typeName<Impl>(), instanceName);
    }

    template<typename API>
    Slot<API> use(std::string_view implName, std::string_view instanceName = "default") {
        noteUse<API>();

        if (!kernel_.registry.hasImpl<API>(implName)) {
            throw Lattice::Exception(tag, "unknown implementation '{}' for '{}'", implName, typeName<API>());
        }

        const auto& entry = kernel_.registry.requireImpl<API>(implName);
        Node* child = nullptr;
        ObjectId objectId = kernel_.objects.find(id, typeName<API>(), instanceName);

        if (ObjectRegistry::valid(objectId)) {
            auto* entry = kernel_.objects.get(objectId);
            child = static_cast<Node*>(entry->object);
            if (child->api) {
                if constexpr (std::is_same_v<API, ServiceAPI>) {
                    static_cast<ServiceAPI*>(child->api)->stop();
                    child->destroy(child->instance);
                    child->instance = nullptr;
                    child->api = nullptr;
                    child->destroy = nullptr;
                    child->configure = nullptr;
                }
            }
            child->children.clear();;
        } else {
            child = makeChild(instanceName, implName);
            // добавляем алиас: <API>("name") -> Impl; <Impl>("name") -> Impl;
            kernel_.objects.alias(child->id, id, typeName<API>(), instanceName);
            Logger::info(tag, "+ interface '{}'", typeName<API>());
        }

        applyRoles(child, entry);

        Logger::info(tag, "> use '{}' = '{}'", typeName<API>(), implName);
        return child;
    }

    // ищет компонент <T> в текущем узле и родительских
    // если компонент не найден возвращает nullptr
    template<typename API>
    Slot<API> find(std::string_view instanceName = "default") {
        noteRequire<API>();
        const auto apiType = typeName<API>();
        ObjectId objectId = kernel_.objects.find(id, apiType, instanceName);

        if (ObjectRegistry::valid(objectId)) {
            if (auto* entry = kernel_.objects.get(objectId)) {
                Node* node = static_cast<Node*>(entry->object);

                if (node && node->getObject())
                    return Slot<API>(node);
            }
        }

        // Для runtime-конфигурации допускаем:
        // ServiceAPI("ClassicMD")
        //     ↓
        // ClassicMD("default")
        // То есть instanceName может фактически быть именем реализации.
        if (kernel_.registry.hasImpl<API>(instanceName)) {
            ObjectId objectId = kernel_.objects.find(
                id,
                instanceName,
                "default"
            );

            if (ObjectRegistry::valid(objectId)) {
                if (auto* entry = kernel_.objects.get(objectId)) {
                    Node* node = static_cast<Node*>(entry->object);

                    if (node && node->getObject())
                        return Slot<API>(node);
                }
            }
        }

        if (parent)
            return parent->find<API>(instanceName);

        return {};
    }

    bool has(std::string_view type, std::string_view name) const {
        return ObjectRegistry::valid(kernel_.objects.find(id, type, name));
    }

    // ищет компонент <T> в текущем узле и родительских
    // если компонент не найден кидает исключение
    template<typename T>
    Ref<T> require(std::string_view instanceName = "default") {
        noteRequire<T>();
        if (auto slot = find<T>(instanceName); slot.exists())
            return Ref<T>(slot.get());

        throw Lattice::Exception(tag, "Component '{}' with instance '{}' not found", typeName<T>(), instanceName);
    }

    // вызывает метод configure() у всех компонентов ветки
    void configureAll() {
        if (configure) {
            Logger::info(tag, "Configuring '{}'", kernel_.objects[id].type);
            configure(instance, *this);
        } else if (instance) {
            Logger::warning(tag, "Component '{}' has no configure callback", kernel_.objects[id].type);
        }

        for (auto& child : children)
            child->configureAll();
    }

    // удаляет компонент <T> из ветки
    template<typename API>
    void remove(std::string_view instanceName = "default") {
        ObjectId objectId = kernel_.objects.find(id, typeName<API>(), instanceName);
        if (!ObjectRegistry::valid(objectId))
            return;

        auto* entry = kernel_.objects.get(objectId);
        if (!entry)
            return;

        Node* node = static_cast<Node*>(entry->object);
        if (!node)
            return;

        auto child = std::find_if(
            children.begin(),
            children.end(),
            [node](const std::unique_ptr<Node>& p) {
                return p.get() == node;
            }
        );

        if (child == children.end())
            return;

        children.erase(child);
    }

    // останавливает все сервисы
    void stopServices() {
        if (kernel_.objects[id].type == typeName<ServiceAPI>() && api) {
            static_cast<ServiceAPI*>(api)->stop();
        }
        
        for (auto& child : children)
            child->stopServices();
    }

    ~Node() {
        stopServices();
        if (destroy)
            destroy(instance);
        instance = nullptr;
        api = nullptr;
        destroy = nullptr;
        configure = nullptr;
        children.clear();
        if (id)
            kernel_.objects.destroy(id);
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

    void dumpTree(std::string_view componentName = "Unknown") const {
        Logger::Tree tree("Root");
        appendTree(tree, 0, 7);
        tree.print();
    }

    void* getObject() const noexcept {
        return api ? api : instance;
    }

    
    template<typename T>
    ObjectId bind(std::string_view name, T* ptr,
                double min = 0, double max = 0, bool hasRange = false) {
        Node* child = makeChild(name, "param");
        kernel_.settings.bind(child->id, ptr, min, max, hasRange);
        return child->id;
    }

    template<typename T, typename F>
    requires std::invocable<F&, T>
    ObjectId bind(std::string_view name, T* ptr, F&& onChange,
                double min = 0, double max = 0, bool hasRange = false) {
        Node* child = makeChild(name, "param");
        kernel_.settings.bind(child->id, ptr, std::forward<F>(onChange), min, max, hasRange);
        return child->id;
    }

    ObjectId on(std::string_view name, std::function<void()> handler) {
        Node* child = makeChild(name, "action");
        kernel_.settings.on(child->id, std::move(handler));
        return child->id;
    }

    ObjectId param(std::string_view name) const {
        ObjectId pid = kernel_.objects.find(id, "param", name);
        if (!ObjectRegistry::valid(pid))
            throw Exception(tag, "param '{}' not found", name);
        return pid;
    }

    ObjectId action(std::string_view name) const {
        ObjectId aid = kernel_.objects.find(id, "action", name);
        if (!ObjectRegistry::valid(aid))
            throw Exception(tag, "action '{}' not found", name);
        return aid;
    }

    template<typename T>
    T get(std::string_view name) const {
        return kernel_.settings.get<T>(param(name));
    }

    template<typename T>
    void set(std::string_view name, T value) {
        kernel_.settings.set(param(name), std::move(value));
    }

    void fire(std::string_view name) const {
        kernel_.settings.fire(action(name));
    }

    void activate(std::string_view role) {
        kernel_.context.set(role, id);
    }

    void onActivate(std::string_view role) {
        on("activate", [this, r = std::string(role)] { activate(r); });
        activate(role);
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
