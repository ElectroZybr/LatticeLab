#include "ScriptAPI.hpp"

#include <algorithm>
#include <cctype>
#include <random>
#include <stdexcept>

#include <sol/sol.hpp>

namespace Lattice {

namespace {

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::string normalizeAtomSymbol(std::string value) {
    value = lowercase(std::move(value));
    if (!value.empty()) {
        value[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(value[0])));
    }
    return value;
}

std::optional<AtomData::Type> findAtomTypeBySymbol(std::string_view speciesName) {
    const std::string atomSymbol = normalizeAtomSymbol(std::string(speciesName));
    for (size_t i = 0; i < static_cast<size_t>(AtomData::Type::COUNT); ++i) {
        const AtomData::Type type = static_cast<AtomData::Type>(i);
        if (AtomData::symbol(type) == atomSymbol) {
            return type;
        }
    }
    return std::nullopt;
}

} // namespace

glm::vec3 readVec3Object(const sol::object& vecObject, const glm::vec3& fallback = glm::vec3(0.0f)) {
    if (!vecObject.is<sol::table>()) {
        return fallback;
    }
    const sol::table vecTable = vecObject.as<sol::table>();
    return glm::vec3(
        vecTable.get_or("x", vecTable.get_or(1, fallback.x)),
        vecTable.get_or("y", vecTable.get_or(2, fallback.y)),
        vecTable.get_or("z", vecTable.get_or(3, fallback.z))
    );
}

bool ScriptBatch::spawn(const std::string& speciesName, const sol::object& positionObject, const sol::object& optionsObject) {
    const glm::vec3 position = readVec3Object(positionObject);
    const SpawnOptions options = parseSpawnOptions(optionsObject);
    if (const std::optional<AtomData::Type> atomType = findAtomTypeBySymbol(speciesName); atomType.has_value()) {
        (void)simulation_.appendAtomFast(position, options.velocity, *atomType, options.fixed);
        return true;
    }
    return simulation_.spawnMolecule(speciesName, position, std::nullopt, options.fixed);
}

bool ScriptBatch::random_spawn(const std::string& speciesName, const sol::object& optionsObject) {
    return simulation_.randomSpawn(speciesName, parseSpawnOptions(optionsObject));
}

void ScriptBatch::finish() {
    if (finished_) {
        return;
    }
    simulation_.finishAtomBatch();
    finished_ = true;
}

SpawnOptions ScriptBatch::parseSpawnOptions(const sol::object& object) const {
    const glm::vec3 worldSize = simulation_.world().getWorldSize();
    SpawnOptions options;
    options.max = worldSize;
    if (!object.is<sol::table>()) {
        return options;
    }

    const sol::table table = object.as<sol::table>();
    if (const sol::object velocity = table["velocity"]; velocity.is<sol::table>()) {
        options.velocity = readVec3Object(velocity);
    }
    if (const sol::object regionObject = table["region"]; regionObject.is<sol::table>()) {
        const sol::table regionTable = regionObject.as<sol::table>();
        options.min = readVec3Object(regionTable["min"], options.min);
        options.max = readVec3Object(regionTable["max"], options.max);
    }
    options.temperature = table.get_or("temperature", options.temperature);
    options.margin = table.get_or("margin", options.margin);
    options.minDistance = table.get_or("min_distance", options.minDistance);
    options.maxAttempts = table.get_or("attempts", options.maxAttempts);
    options.randomRotation = table.get_or("random_rotation", options.randomRotation);
    options.fixed = table.get_or("fixed", options.fixed);
    return options;
}

void ScriptAPI::clear() { simulation_.clear(); }

void ScriptAPI::set_world_title(const std::string& title) { simulation_.setWorldTitle(title); }

void ScriptAPI::set_box(float x, float y, sol::variadic_args args) {
    const float z = args.size() >= 1 ? static_cast<float>(args.get<float>(0)) : 6.0f;
    simulation_.setSizeBox(glm::vec3(x, y, z));
}

std::tuple<float, float, float> ScriptAPI::world_size() const {
    const glm::vec3 size = simulation_.world().getWorldSize();
    return {size.x, size.y, size.z};
}

std::tuple<int, std::vector<std::string>> ScriptAPI::load_molecules(const std::string& path) {
    std::vector<std::string> loadedNames = loadMoleculesFrom(path);
    return {static_cast<int>(loadedNames.size()), std::move(loadedNames)};
}

std::shared_ptr<ScriptBatch> ScriptAPI::begin_batch() {
    simulation_.beginAtomBatch();
    return std::make_shared<ScriptBatch>(simulation_);
}

float ScriptAPI::lj_min(const std::string& speciesA, const std::string& speciesB) const {
    const std::optional<AtomData::Type> atomA = findAtomTypeBySymbol(speciesA);
    const std::optional<AtomData::Type> atomB = findAtomTypeBySymbol(speciesB);
    if (!atomA.has_value() || !atomB.has_value()) {
        throw std::runtime_error("scene:lj_min supports atoms only");
    }
    return simulation_.lj_min(*atomA, *atomB);
}

std::vector<std::string> ScriptAPI::loadMoleculesFrom(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("molecule path does not exist: '" + path.string() + "'");
    }

    std::vector<std::filesystem::path> pdbFiles;
    if (std::filesystem::is_regular_file(path)) {
        if (lowercase(path.extension().string()) != ".pdb") {
            throw std::runtime_error("molecule file must have .pdb extension: '" + path.string() + "'");
        }
        pdbFiles.push_back(path);
    } else if (std::filesystem::is_directory(path)) {
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            if (lowercase(entry.path().extension().string()) != ".pdb") {
                continue;
            }
            pdbFiles.push_back(entry.path());
        }
    } else {
        throw std::runtime_error("molecule path must be a .pdb file or directory: '" + path.string() + "'");
    }

    std::sort(pdbFiles.begin(), pdbFiles.end());

    std::vector<std::string> loadedNames;
    loadedNames.reserve(pdbFiles.size());
    for (const std::filesystem::path& pdbPath : pdbFiles) {
        const std::string moleculeName = pdbPath.stem().string();
        if (moleculeName.empty()) {
            continue;
        }

        simulation_.loadMoleculeTemplate(moleculeName, pdbPath);
        loadedNames.push_back(moleculeName);
    }

    return loadedNames;
}

} // namespace Lattice
