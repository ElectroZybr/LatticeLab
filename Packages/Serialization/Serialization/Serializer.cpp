#include "Serializer.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace Serialization {
using namespace Lattice;
namespace {
using Error = Exception<Serializer>;

// Value's containers are shared. Copy recursively so a failed save does not mutate
// either the retained document or a parameter's nested values.
Value copy(const Value& value) {
    if (value.is<Object>()) {
        if (!std::get<ObjectPtr>(value)) throw Error("Configuration cannot contain null objects");
        Object result;
        for (const auto& [name, item] : value.require<Object>()) result.emplace(name, copy(item));
        return result;
    }
    if (value.is<Array>()) {
        if (!std::get<ArrayPtr>(value)) throw Error("Configuration cannot contain null arrays");
        Array result;
        for (const auto& item : value.require<Array>()) result.push_back(copy(item));
        return result;
    }
    if (value.is<std::string>()) {
        if (!std::get<StringPtr>(value)) throw Error("Configuration cannot contain null strings");
        return value.require<std::string>();
    }
    return value;
}

Object& object(Value& value, const std::string& path) {
    if (!value.is<Object>() || !std::get<ObjectPtr>(value)) throw Error("'{}' must be a table", path);
    return *std::get<ObjectPtr>(value);
}

// A single ':' separates type and instance; '::' remains part of a C++ type.
size_t separator(std::string_view key) {
    for (size_t i = 0; i < key.size(); ++i)
        if (key[i] == ':' && (i == 0 || key[i - 1] != ':') &&
            (i + 1 == key.size() || key[i + 1] != ':')) return i;
    return std::string_view::npos;
}

double number(const Value& value, const std::string& path) {
    if (value.is<int64_t>()) return double(value.require<int64_t>());
    if (value.is<double>()) return value.require<double>();
    throw Error("'{}' must be numeric", path);
}

template<class Vector>
Value vector(const Value& value, const std::string& path) {
    if (!value.is<Array>()) throw Error("'{}' must be an array", path);
    const auto& array = value.require<Array>();
    Vector result{};
    if (array.size() != std::size_t(result.length())) throw Error("'{}' has the wrong vector size", path);
    for (int i = 0; i < result.length(); ++i) {
        const auto item = number(array[i], path);
        if (!std::isfinite(item) || std::abs(item) > std::numeric_limits<float>::max())
            throw Error("'{}' contains an out-of-range vector element", path);
        result[i] = float(item);
    }
    return result;
}

Value parameter(const Value& supplied, const Value& current, const std::string& path) {
    if (supplied.index() == current.index()) return copy(supplied);
    if (current.is<double>() && supplied.is<int64_t>()) return double(supplied.require<int64_t>());
    if (current.is<int64_t>() && supplied.is<double>()) {
        const auto value = supplied.require<double>();
        if (std::isfinite(value) && std::trunc(value) == value &&
            static_cast<long double>(value) >= std::numeric_limits<int64_t>::min() &&
            static_cast<long double>(value) <= std::numeric_limits<int64_t>::max())
            return int64_t(value);
    }
    if (current.is<glm::vec2>()) return vector<glm::vec2>(supplied, path);
    if (current.is<glm::vec3>()) return vector<glm::vec3>(supplied, path);
    if (current.is<glm::vec4>()) return vector<glm::vec4>(supplied, path);
    throw Error("Parameter '{}' has an incompatible value type", path);
}
}

Serializer::Serializer(NodeSystem& nodes, NodeId root, IOSubsystem& io)
    : nodes_(nodes), root_(root), generation_(nodes.registry.generation(root)), io_(io),
      observer_(nodes.exports.observeParameters([this](ExportId id, ParamChange change) {
          changed(id, change);
      })) {}

Serializer::~Serializer() { nodes_.exports.unobserveParameters(observer_); }

void Serializer::changed(ExportId id, ParamChange change) {
    if (change != ParamChange::Set) {
        overrides_.erase(id);
        return;
    }
    const auto* root = nodes_.registry.get(root_);
    if (!root || root->state != NodeState::Active || nodes_.registry.generation(root_) != generation_)
        return;
    const auto* entry = nodes_.exports.get(id);
    if (!entry || entry->owner == InvalidNodeId) return;
    auto owner = entry->owner;
    while (owner != InvalidNodeId && owner != root_)
        owner = nodes_.registry.require(owner).parent;
    if (owner == root_) overrides_.insert_or_assign(id, copy(nodes_.exports.value(id)));
}

bool Serializer::declared(NodeId owner) const {
    const auto found = declarations_.find(owner);
    return found != declarations_.end() && found->second == nodes_.registry.generation(owner);
}

std::string Serializer::key(NodeId owner) const {
    const auto& node = nodes_.registry.require(owner);
    const auto* blueprint = nodes_.blueprints.get(node.bp);
    const std::string type = blueprint ? blueprint->name : "";
    // A composition boundary carries its type so a missing branch can be rebuilt.
    if (blueprint && blueprint->meta.compose)
        return node.name.empty() ? type : type + ":" + node.name;
    if (!node.name.empty() && separator(node.name) == std::string_view::npos &&
        nodes_.exports.find(node.parent, node.name) == InvalidExportId) {
        size_t matches = 0;
        for (auto id : nodes_.registry.children(node.parent)) {
            const auto& sibling = nodes_.registry.require(id);
            const auto* siblingType = nodes_.blueprints.get(sibling.bp);
            if (sibling.name == node.name || (sibling.name.empty() && siblingType &&
                (siblingType->name == node.name || siblingType->shortName() == node.name))) ++matches;
        }
        if (matches == 1) return node.name;
    }
    return type + ":" + node.name;
}

Object Serializer::layer(NodeId owner) {
    if (nodes_.registry.require(owner).state != NodeState::Active) return {};
    Object result;
    for (const auto& [id, value] : overrides_) {
        const auto* entry = nodes_.exports.get(id);
        if (entry && entry->owner == owner) result.emplace(entry->name, copy(value));
    }
    for (auto id : nodes_.registry.children(owner)) {
        if (nodes_.registry.require(id).state != NodeState::Active) continue;
        auto config = layer(id);
        if (config.empty() && !declared(id)) continue;
        const auto name = key(id);
        if (!result.emplace(name, std::move(config)).second)
            throw Error("Configuration key '{}' collides with another child or parameter", name);
    }
    return result;
}

void Serializer::checkRoot() const {
    const auto* node = nodes_.registry.get(root_);
    if (!node || node->state != NodeState::Active || nodes_.registry.generation(root_) != generation_)
        throw Error("Configuration root has expired or is retiring");
}

void Serializer::load(std::optional<std::string> path) {
    checkRoot();
    const std::filesystem::path input = path ? std::filesystem::path(*path) :
        (document_ ? path_ : std::filesystem::path("User/runtime.yaml"));
    // A first launch has no saved user layer yet. Explicitly requested or
    // previously selected missing files still report normal IO errors.
    if (!path && !document_ && !std::filesystem::exists(input)) {
        path_ = input;
        document_.emplace();
        return;
    }
    auto document = io_.read(input);
    auto* section = document.get("Lattice");
    if (!section) throw Error("'{}' has no [Lattice] section", input.string());
    visit(root_, object(*section, "Lattice"), "Lattice");
    path_ = input;
    document.root().erase("Lattice"); // Retain only unrelated sections; the layer owns explicit values.
    document_ = std::move(document);
}

std::string Serializer::status() const {
    checkRoot();
    if (!document_) return "Configuration has not been loaded";
    return std::format("Config: {} ({} overrides)", path_.string(), overrides_.size());
}

void Serializer::save(std::optional<std::string> path) {
    checkRoot();
    const std::filesystem::path output = path ? std::filesystem::path(*path) :
        (document_ ? path_ : std::filesystem::path("User/runtime.yaml"));
    auto value = document_ ? copy(Value{document_->root()}) : Value{Object{}};
    Document document{value.get<Object>()};
    document.root().insert_or_assign("Lattice", layer(root_));
    if (!output.parent_path().empty()) std::filesystem::create_directories(output.parent_path());
    io_.write(output, document);
    path_ = output;
    document.root().erase("Lattice"); // Retain only unrelated sections; the layer owns explicit values.
    document_ = std::move(document);
}

NodeId Serializer::child(NodeId parent, std::string_view key, const std::string& path) {
    const auto split = separator(key);
    const bool typed = split != std::string_view::npos;
    const auto typeName = typed ? key.substr(0, split) : key;
    const auto name = typed ? key.substr(split + 1) : key;
    BlueprintId type = InvalidBlueprintId;
    if (!typeName.empty()) {
        type = nodes_.blueprints.find(typeName);
        // Plain instance names do not need any type lookup. Resolve short type
        // names only if no matching named child was found below.
        if (typed && type == InvalidBlueprintId) type = nodes_.blueprints.resolve(typeName);
        if (typed && type == InvalidBlueprintId) throw Error("Unknown blueprint '{}' at '{}'", typeName, path);
    }
    std::vector<NodeId> matching;
    bool named = false;
    for (auto id : nodes_.registry.children(parent)) {
        const auto& node = nodes_.registry.require(id);
        if (node.name == name) named = true;
        if (typed ? node.name == name && node.bp == type :
            node.name == key || (node.name.empty() && type != InvalidBlueprintId && node.bp == type))
            matching.push_back(id);
    }
    if (!typed && matching.empty() && type == InvalidBlueprintId) {
        type = nodes_.blueprints.resolve(typeName);
        for (auto id : nodes_.registry.children(parent)) {
            const auto& node = nodes_.registry.require(id);
            if (node.name.empty() && type != InvalidBlueprintId && node.bp == type) matching.push_back(id);
        }
    }
    if (matching.size() > 1) throw Error("Branch '{}' is ambiguous; use Type:name", path);
    NodeId id;
    if (matching.size() == 1) {
        id = matching.front();
        if (nodes_.registry.require(id).state != NodeState::Active)
            throw Error("Branch '{}' is retiring; retry after maintain()", path);
    } else {
        if (typed && named && !name.empty()) throw Error("Blueprint does not match existing branch '{}'", path);
        const auto* blueprint = nodes_.blueprints.get(type);
        if (!blueprint || !blueprint->meta.compose)
            throw Error("Missing branch '{}' needs a composable Type:name key", path);
        id = nodes_.builder.add(parent, type, typed ? name : std::string_view{});
    }
    const auto* blueprint = nodes_.blueprints.get(nodes_.registry.require(id).bp);
    if (blueprint && blueprint->meta.compose)
        declarations_.insert_or_assign(id, nodes_.registry.generation(id));
    return id;
}

void Serializer::visit(NodeId owner, Object& config, const std::string& path) {
    std::vector<std::string> children;
    // Resolve exported names before considering objects as child branches: an
    // Object-valued parameter is still one parameter, with no reserved wrappers.
    for (const auto& [name, value] : config) {
        const auto id = nodes_.exports.find(owner, name);
        if (id == InvalidExportId) {
            children.push_back(name);
            continue;
        }
        if (nodes_.exports.get(id)->kind != ExportKind::Param)
            throw Error("'{}.{}' is an action, not a parameter", path, name);
        auto converted = parameter(value, nodes_.exports.value(id), path + "." + name);
        nodes_.exports.set(id, converted);
    }
    std::ranges::sort(children);
    std::unordered_set<NodeId> visited;
    for (const auto& name : children) {
        const auto childPath = path + "/" + name;
        auto& branch = object(config.at(name), childPath);
        const auto id = child(owner, name, childPath);
        if (!visited.insert(id).second) throw Error("Duplicate child at '{}'", childPath);
        visit(id, branch, childPath);
    }
}
}
