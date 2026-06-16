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

namespace Lattice {

class ScriptBatch {
public:
    explicit ScriptBatch(Simulation& simulation) : simulation_(simulation) {}

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
    void set_box(float x, float y, sol::variadic_args args);
    std::tuple<int, std::vector<std::string>> load_molecules(const std::string& path);
    std::shared_ptr<ScriptBatch> begin_batch();

private:
    std::vector<std::string> loadMoleculesFrom(const std::filesystem::path& path);

    Simulation& simulation_;
};

} // namespace Lattice
