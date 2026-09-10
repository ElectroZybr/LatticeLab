#include <Lattice/Kernel/Requirements.hpp>
#include <Lattice/Tools/Logger.hpp>
#include "Lattice/Kernel/Objects.hpp"
#include "Lattice/Tools/LogTree.hpp"

#include <format>
#include <unordered_set>

namespace Lattice {
namespace {

const PluginCatalog* catalogFor(std::string_view name, Objects& objects, ObjectId blueprintsId) {
    ObjectId id = objects.find(name, blueprintsId);

    if (!Objects::valid(id))
        return nullptr;

    for (const auto& catalog : pluginCatalogs())
        for (ObjectId provided : catalog.provided)
            if (provided == id)
                return &catalog;

    return nullptr;
}

std::vector<std::string> collectUniqueList(std::string_view name, Objects& objects, ObjectId blueprintsId) {
    std::vector<std::string> result;
    std::unordered_set<std::string> seen;

    auto walk = [&](auto&& self, std::string_view currentName) -> void {
        const PluginCatalog* catalog = catalogFor(currentName, objects, blueprintsId);
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

void appendComposition(Logger::Tree& tree, std::string_view name, size_t depth, Objects& objects, ObjectId blueprintsId, std::unordered_set<std::string>& seen) {
    if (!seen.insert(std::string(name)).second)
        return;

    const PluginCatalog* catalog = catalogFor(name, objects, blueprintsId);
    if (!catalog)
        return;

    for (const auto& dep : catalog->deps) {
        tree.node(dep.type, depth);

        if (dep.kind == DepKind::Add)
            appendComposition(tree, dep.type, depth + 1, objects, blueprintsId, seen);
    }
}

} // namespace

std::vector<std::string> uniqueList(std::string_view name, Objects& objects, ObjectId blueprintsId) {
    if (!Objects::valid(objects.find(name, blueprintsId))) {
        Logger::error(tag, "unknown component '{}'", name);
        return {};
    }

    if (!catalogFor(name, objects, blueprintsId)) {
        Logger::error(tag, "no compile catalog for '{}'", name);
        return {};
    }

    return collectUniqueList(name, objects, blueprintsId);
}

std::vector<std::string> printUniqueList(std::string_view name, Objects& objects, ObjectId blueprintsId) {
    const auto requirements = collectUniqueList(name, objects, blueprintsId);

    Logger::Tree tree{"Dependencies"};

    for (const auto& requirement : requirements) {
        const bool exists = Objects::valid(objects.find(requirement, blueprintsId));

        tree.node(std::format("{}{}", exists ? Color::paint("✓ ", Color::ok) : Color::paint("✗ ", Color::error), requirement));
    }

    tree.print();
    return requirements;
}

void printCompositionTree(std::string_view name, Objects& objects, ObjectId blueprintsId) {
    if (!catalogFor(name, objects, blueprintsId)) {
        Logger::error(tag, "no compile catalog for '{}'", name);
        return;
    }

    Logger::Tree tree{std::string(name)};
    std::unordered_set<std::string> seen;

    appendComposition(tree, name, 0, objects, blueprintsId, seen);
    tree.print();
}

bool check(std::string_view name, Objects& objects, ObjectId blueprintsId) {
    if (!Objects::valid(objects.find(name, blueprintsId))) {
        Logger::error(tag, "unknown component '{}'", name);
        return false;
    }

    if (!catalogFor(name, objects, blueprintsId)) {
        Logger::error(tag, "no compile catalog for '{}'", name);
        return false;
    }

    const auto requirements = collectUniqueList(name, objects, blueprintsId);

    for (const auto& requirement : requirements) {
        if (!Objects::valid(objects.find(requirement, blueprintsId))) {
            Logger::error(tag, "{} check failed", name);
            return false;
        }
    }

    Logger::ok(tag, "{} check passed", name);
    printCompositionTree(name, objects, blueprintsId);
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