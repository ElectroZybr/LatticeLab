#include "Serializer.hpp"
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

std::string text(const Object& config, std::string_view field, const std::string& path) {
    const auto found = config.find(std::string(field));
    if (found == config.end() || !found->second.is<std::string>())
        throw Error("'{}.{}' must be a string", path, field);
    return found->second.require<std::string>();
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
    : nodes_(nodes), root_(root), generation_(nodes.registry.generation(root)), io_(io) {}

void Serializer::checkRoot() const {
    const auto* node = nodes_.registry.get(root_);
    if (!node || node->state != NodeState::Active || nodes_.registry.generation(root_) != generation_)
        throw Error("Configuration root has expired or is retiring");
}

void Serializer::load(std::optional<std::string> path) {
    checkRoot();
    const std::filesystem::path input = path ? std::filesystem::path(*path) :
        (document_ ? path_ : std::filesystem::path("Config/runtime.toml"));
    auto document = io_.read(input);
    auto* section = document.get("Lattice");
    if (!section) throw Error("'{}' has no [Lattice] section", input.string());
    visit(root_, object(*section, "Lattice"), true, "Lattice");
    path_ = input;
    document_ = std::move(document);
}

void Serializer::save(std::optional<std::string> path) {
    checkRoot();
    if (!document_) throw Error("Load a configuration before saving");
    const std::filesystem::path output = path ? std::filesystem::path(*path) : path_;
    auto value = copy(Value{document_->root()});
    Document document{value.get<Object>()};
    visit(root_, object(*document.get("Lattice"), "Lattice"), false, "Lattice");
    io_.write(output, document);
    path_ = output;
    document_ = std::move(document);
}

NodeId Serializer::child(NodeId parent, const Object& config, bool create, const std::string& path) {
    const auto name = text(config, "name", path);
    const auto typeIt = config.find("blueprint");
    BlueprintId type = InvalidBlueprintId;
    if (typeIt != config.end()) {
        const auto typeName = text(config, "blueprint", path);
        type = nodes_.blueprints.resolve(typeName);
        if (type == InvalidBlueprintId) throw Error("Unknown blueprint '{}' at '{}'", typeName, path);
    }
    std::vector<NodeId> named;
    std::vector<NodeId> matching;
    for (auto id : nodes_.registry.children(parent)) {
        const auto& node = nodes_.registry.require(id);
        if (node.name != name) continue;
        named.push_back(id);
        if (type == InvalidBlueprintId || node.bp == type) matching.push_back(id);
    }
    if (matching.size() > 1) throw Error("Branch '{}' is ambiguous; specify its blueprint", path);
    if (matching.size() == 1) {
        const auto id = matching.front();
        if (nodes_.registry.require(id).state != NodeState::Active)
            throw Error("Branch '{}' is retiring; retry after maintain()", path);
        return id;
    }
    if (!named.empty()) throw Error("Blueprint does not match existing branch '{}'", path);
    if (!create) throw Error("Branch '{}' no longer exists", path);
    if (type == InvalidBlueprintId) throw Error("Missing branch '{}' needs a blueprint", path);
    return nodes_.builder.add(parent, type, name);
}

void Serializer::visit(NodeId owner, Object& config, bool loading, const std::string& path) {
    for (const auto& [key, value] : config)
        if (key != "name" && key != "blueprint" && key != "params" && key != "children")
            throw Error("Unknown configuration field '{}.{}'", path, key);

    if (auto found = config.find("params"); found != config.end()) {
        auto& params = object(found->second, path + ".params");
        for (auto& [name, value] : params) {
            const auto id = nodes_.exports.find(owner, name);
            const auto* entry = nodes_.exports.get(id);
            if (!entry || entry->kind != ExportKind::Param)
                throw Error("Parameter '{}.{}' is not exported", path, name);
            if (loading) {
                auto converted = parameter(value, nodes_.exports.value(id), path + "." + name);
                nodes_.exports.set(id, converted);
            } else value = copy(nodes_.exports.value(id));
        }
    }
    if (auto found = config.find("children"); found != config.end()) {
        if (!found->second.is<Array>()) throw Error("'{}.children' must be an array", path);
        std::unordered_set<std::string> names;
        // Ordered array: providers may be declared before branches which need them.
        for (auto& item : *std::get<ArrayPtr>(found->second)) {
            auto& configChild = object(item, path + ".children");
            const auto name = text(configChild, "name", path);
            if (!names.insert(name).second) throw Error("Duplicate child '{}' at '{}'", name, path);
            const auto childPath = path + "/" + name;
            const auto id = child(owner, configChild, loading, childPath);
            visit(id, configChild, loading, childPath);
        }
    }
}
}
