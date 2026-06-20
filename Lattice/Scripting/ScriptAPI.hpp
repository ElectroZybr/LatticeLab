#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <glm/glm.hpp>
#include <sol/forward.hpp>

#include "Lattice/Engine/Simulation.h"
#include "Lattice/Generators/LatticeFill.hpp"
#include "Lattice/Generators/RandomFill.hpp"

namespace Lattice {

class ScriptBatch {
public:
    explicit ScriptBatch(Simulation& simulation) : simulation_(simulation) {}

    bool spawn(const std::string& speciesName, const sol::object& positionObject, const sol::object& optionsObject);
    bool random_spawn(const std::string& speciesName, const sol::object& optionsObject);
    void finish();

private:
    [[nodiscard]] SpawnOptions parseSpawnOptions(const sol::object& object) const;

    Simulation& simulation_;
    bool finished_ = false;
};

class ScriptAPI {
public:
    explicit ScriptAPI(Simulation& simulation) : simulation_(simulation) {}

    void clear();
    void set_world_title(const std::string& title);
    void set_box(float x, float y, sol::variadic_args args);
    std::tuple<float, float, float> world_size() const;
    std::tuple<int, std::vector<std::string>> load_molecules(const std::string& path);
    std::shared_ptr<ScriptBatch> begin_batch();
    float lj_min(const std::string& speciesA, const std::string& speciesB) const;
    int random_fill(const sol::object& regionObject, const sol::object& compositionObject, const sol::object& optionsObject);
    int lattice_fill(const sol::object& regionObject, const sol::object& compositionObject, const sol::object& optionsObject);

private:
    [[nodiscard]] std::vector<::Generators::Compose> parseComposition(const sol::object& object, bool requireFraction) const;
    [[nodiscard]] std::unique_ptr<Generators::Region> parseRegion(const sol::object& object) const;
    [[nodiscard]] ::Generators::RandomFillOptions parseRandomFillOptions(const sol::object& object) const;
    [[nodiscard]] ::Generators::LatticeFillOptions parseLatticeFillOptions(const sol::object& object) const;
    std::vector<std::string> loadMoleculesFrom(const std::filesystem::path& path);

    Simulation& simulation_;
};

} // namespace Lattice
