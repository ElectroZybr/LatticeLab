#include <Lattice/Kernel/Blueprints.hpp>

#include <algorithm>
#include <format>

#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Tools/Logger.hpp>
#include <Lattice/Tools/TreeFormatter.hpp>

namespace Lattice {

std::string_view Blueprint::shortName() const noexcept {
    const auto pos = name.rfind("::");
    return std::string_view(name).substr(pos == std::string::npos ? 0 : pos + 2);
}

std::string_view Blueprint::namespaceName() const noexcept {
    const auto pos = name.rfind("::");
    return pos == std::string::npos
        ? std::string_view{}
        : std::string_view(name).substr(0, pos);
}

BlueprintId Blueprints::add(std::string_view name, std::initializer_list<BlueprintId> bases) {
    validateName(name);

    std::vector<BlueprintId> uniqueBases;
    uniqueBases.reserve(bases.size());

    for (BlueprintId base : bases) {
        require(base);

        if (std::find(uniqueBases.begin(), uniqueBases.end(), base) == uniqueBases.end())
            uniqueBases.push_back(base);
    }

    Blueprint blueprint{
        .name = std::string(name),
        .bases = std::move(uniqueBases)
    };

    return Base::create(std::move(blueprint), std::string(name));
}

BlueprintId Blueprints::resolveImplementation(
    BlueprintId api,
    std::string_view nameSpace,
    std::string_view descriptor
) const {
    require(api);

    const auto compatible = [&](BlueprintId id) {
        const Blueprint* blueprint = get(id);

        return blueprint &&
               blueprint->meta.create &&
               isA(id, api) &&
               (descriptor.empty() || blueprint->meta.descriptor == descriptor);
    };

    if (compatible(api))
        return api;

    const auto select = [&](bool local) {
        BlueprintId result = InvalidId;

        for (BlueprintId id = 0; id < size(); ++id) {
            const Blueprint* blueprint = get(id);

            if (!blueprint || !compatible(id))
                continue;

            if (local && blueprint->namespaceName() != nameSpace)
                continue;

            if (result != InvalidId)
                throw Exception(
                    "Blueprints",
                    "Multiple implementations of '{}': '{}' and '{}'",
                    require(api).name,
                    require(result).name,
                    blueprint->name
                );

            result = id;
        }

        return result;
    };

    if (!nameSpace.empty()) {
        const BlueprintId local = select(true);

        if (local != InvalidId)
            return local;
    }

    const BlueprintId result = select(false);

    if (result == InvalidId)
        throw Exception(
            "Blueprints",
            "No constructible implementation of '{}'",
            require(api).name
        );

    return result;
}

void* Blueprints::cast(BlueprintId from, BlueprintId to, void* object) const {
    if (!object || !get(from) || !get(to))
        return nullptr;

    void* result = nullptr;

    auto walk = [&](auto&& self, BlueprintId id, void* ptr) -> void {
        if (id == to) {
            if (result && result != ptr)
                throw Exception(
                    "Blueprints",
                    "Ambiguous conversion '{}' -> '{}'",
                    require(from).name,
                    require(to).name
                );

            result = ptr;
            return;
        }

        const Blueprint& blueprint = require(id);

        for (size_t i = 0; i < blueprint.upcasts.size(); ++i)
            if (blueprint.upcasts[i])
                self(self, blueprint.bases[i], blueprint.upcasts[i](ptr));
    };

    walk(walk, from, object);
    return result;
}

BlueprintId Blueprints::find(std::string_view name, std::string_view nameSpace) const {
    if (nameSpace.empty())
        return Base::find(std::string(name));

    return Base::find(std::string(nameSpace) + "::" + std::string(name));
}

BlueprintId Blueprints::resolve(std::string_view name) const {
    if (name.find("::") != std::string_view::npos)
        return find(name);

    BlueprintId result = InvalidId;

    for (BlueprintId id = 0; id < size(); ++id) {
        const Blueprint* blueprint = get(id);

        if (!blueprint || blueprint->shortName() != name)
            continue;

        if (result != InvalidId)
            throw Exception(
                "Blueprints",
                "Ambiguous name '{}': '{}' and '{}'",
                name,
                require(result).name,
                blueprint->name
            );

        result = id;
    }

    return result;
}

void Blueprints::addBase(BlueprintId derived, BlueprintId base) {
    require(derived);
    require(base);

    if (isA(base, derived))
        throw Exception(
            "Blueprints",
            "Inheritance cycle: '{}' -> '{}'",
            require(derived).name,
            require(base).name
        );

    auto& bases = Base::require(derived).bases;

    if (std::find(bases.begin(), bases.end(), base) == bases.end())
        bases.push_back(base);
}

bool Blueprints::isA(BlueprintId derived, BlueprintId base) const {
    if (!get(derived) || !get(base))
        return false;

    std::vector<bool> visited(size(), false);
    std::vector<BlueprintId> pending{derived};

    while (!pending.empty()) {
        const BlueprintId id = pending.back();
        pending.pop_back();

        if (id == base)
            return true;

        if (visited[id])
            continue;

        visited[id] = true;

        const auto& bases = require(id).bases;
        pending.insert(pending.end(), bases.begin(), bases.end());
    }

    return false;
}

std::vector<BlueprintId> Blueprints::getImpls(BlueprintId api, bool constructibleOnly) const {
    require(api);

    std::vector<BlueprintId> result;

    for (BlueprintId id = 0; id < size(); ++id) {
        const Blueprint* blueprint = get(id);

        if (!blueprint || id == api || !isA(id, api))
            continue;

        if (constructibleOnly && !blueprint->meta.create)
            continue;

        result.push_back(id);
    }

    return result;
}

void Blueprints::dumpTree() const {
    Lattice::TreeFormatter tree("Blueprints");

    auto append = [&](auto&& self, BlueprintId id, size_t depth) -> void {
        tree.node(
            std::format("{} <c>B</> <gr>#{}</>", require(id).name, id),
            depth
        );

        for (BlueprintId child = 0; child < size(); ++child) {
            const Blueprint* blueprint = get(child);

            if (!blueprint)
                continue;

            if (std::find(blueprint->bases.begin(), blueprint->bases.end(), id) != blueprint->bases.end())
                self(self, child, depth + 1);
        }
    };

    for (BlueprintId id = 0; id < size(); ++id) {
        const Blueprint* blueprint = get(id);

        if (blueprint && blueprint->bases.empty())
            append(append, id, 0);
    }

    Logger::message(tree.format());
    Logger::blank();
}

void Blueprints::validateName(std::string_view name) {
    if (name.empty())
        throw Exception("Blueprints", "Blueprint name must not be empty");

    size_t begin = 0;

    while (begin < name.size()) {
        size_t end = name.find("::", begin);

        if (end == std::string_view::npos)
            end = name.size();

        const std::string_view part = name.substr(begin, end - begin);

        if (part.empty() || part.find_first_of(": \t\r\n") != std::string_view::npos)
            throw Exception("Blueprints", "Invalid qualified name '{}'", name);

        if (end == name.size())
            return;

        begin = end + 2;
    }

    throw Exception("Blueprints", "Invalid qualified name '{}'", name);
}

} // namespace Lattice
