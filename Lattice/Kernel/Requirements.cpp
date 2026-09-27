#include <Lattice/Kernel/Requirements.hpp>
#include <Lattice/Tools/Logger.hpp>
#include "Lattice/Tools/TreeFormatter.hpp"

#include <format>
#include <unordered_set>

namespace Lattice {
namespace {

const PluginCatalog* catalogFor(std::string_view name, const Blueprints& blueprints) {
    BlueprintId id = blueprints.resolve(name);

    if (id == Blueprints::InvalidId)
        return nullptr;

    for (const auto& catalog : pluginCatalogs())
        for (BlueprintId provided : catalog.provided)
            if (provided == id)
                return &catalog;

    return nullptr;
}

std::vector<std::string> collectUniqueList(std::string_view name, const Blueprints& blueprints) {
    std::vector<std::string> result;
    std::unordered_set<std::string> seen;
    std::unordered_set<std::string> visited;

    auto walk = [&](auto&& self, std::string_view currentName) -> void {
        if (!visited.insert(std::string(currentName)).second) return;
        const PluginCatalog* catalog = catalogFor(currentName, blueprints);
        if (!catalog)
            return;

        for (const auto& dep : catalog->deps) {
            if (dep.kind == DepKind::Require) {
                if (seen.insert(dep.type).second)
                    result.push_back(dep.type);
            } else if (dep.kind == DepKind::Add) {
                self(self, dep.type);
            }
        }
    };

    walk(walk, name);
    return result;
}

void appendComposition(Lattice::TreeFormatter& tree, std::string_view name, size_t depth, const Blueprints& blueprints, std::unordered_set<std::string>& seen) {
    if (!seen.insert(std::string(name)).second)
        return;

    const PluginCatalog* catalog = catalogFor(name, blueprints);
    if (!catalog)
        return;

    for (const auto& dep : catalog->deps) {
        tree.node(dep.type, depth);

        if (dep.kind == DepKind::Add)
            appendComposition(tree, dep.type, depth + 1, blueprints, seen);
    }
}

} // namespace

std::vector<std::string> uniqueList(std::string_view name, const Blueprints& blueprints) {
    if (blueprints.resolve(name) == Blueprints::InvalidId) {
        Logger::error(tag, "unknown component '{}'", name);
        return {};
    }

    if (!catalogFor(name, blueprints)) {
        Logger::error(tag, "no compile catalog for '{}'", name);
        return {};
    }

    return collectUniqueList(name, blueprints);
}

std::vector<std::string> printUniqueList(std::string_view name, const Blueprints& blueprints) {
    const auto requirements = collectUniqueList(name, blueprints);

    Lattice::TreeFormatter tree{"Dependencies"};

    for (const auto& requirement : requirements) {
        const bool exists = (blueprints.resolve(requirement) != Blueprints::InvalidId);

        tree.node(std::format("{}{}", exists ? "<ok>✓ </>" : "<err>✗ </>", requirement));
    }

    Logger::message(tree.format());
    Logger::blank();
    return requirements;
}

void printCompositionTree(std::string_view name, const Blueprints& blueprints) {
    if (!catalogFor(name, blueprints)) {
        Logger::error(tag, "no compile catalog for '{}'", name);
        return;
    }

    Lattice::TreeFormatter tree{std::string(name)};
    std::unordered_set<std::string> seen;

    appendComposition(tree, name, 0, blueprints, seen);
    Logger::message(tree.format());
    Logger::blank();
}

bool check(std::string_view name, const Blueprints& blueprints) {
    if (blueprints.resolve(name) == Blueprints::InvalidId) {
        Logger::error(tag, "unknown component '{}'", name);
        return false;
    }

    if (!catalogFor(name, blueprints)) {
        Logger::error(tag, "no compile catalog for '{}'", name);
        return false;
    }

    const auto requirements = collectUniqueList(name, blueprints);

    for (const auto& requirement : requirements) {
        if (!(blueprints.resolve(requirement) != Blueprints::InvalidId)) {
            Logger::error(tag, "{} check failed", name);
            return false;
        }
    }

    Logger::ok(tag, "{} check passed", name);
    printCompositionTree(name, blueprints);
    return true;
}

std::vector<CompileDep>& compileDepSink() {
    static std::vector<CompileDep> sink;
    return sink;
}

std::vector<PluginCatalog>& pluginCatalogs() {
    static std::vector<PluginCatalog> catalogs;
    return catalogs;
}

void recordPluginCatalog(PluginCatalog catalog) {
    Logger::info(tag, "plugin '{}' compile deps: {}", catalog.pluginId, catalog.deps.size());
    pluginCatalogs().push_back(std::move(catalog));
}

} // namespace Lattice
