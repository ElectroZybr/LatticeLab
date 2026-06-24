#include <filesystem>

#include "Lattice/Engine/Simulation.h"
#include "Lattice/Scripting/LuaState.h"
#include "Lattice/tests/TestSupport.h"

namespace {
    void testLuaSceneObjectLoadsMoleculesDirectory() {
        Lattice::Simulation simulation;
        simulation.createWorld(glm::vec3(20.0f, 20.0f, 20.0f));

        Lattice::LuaState luaState;
        expect(luaState.valid(), "Lua state should be created");
        luaState.bindSimulation(simulation);

        const bool ok = luaState.runString(R"(
            local count, names = scene:load_molecules("Mods/Base/Molecules")
            assert(count >= 2)
            assert(#names >= 2)
            assert(atoms.H == "H")
            assert(molecule.h2o == "h2o")
        )");
        expect(ok, luaState.lastError());
    }

    void testLuaDslSimulationWorldGasBuildsScene() {
        Lattice::Simulation simulation;
        simulation.createWorld(glm::vec3(20.0f, 20.0f, 20.0f));

        Lattice::LuaState luaState;
        expect(luaState.valid(), "Lua state should be created");
        luaState.bindSimulation(simulation);

        const bool ok = luaState.runString(R"(
            dofile("Mods/Base/API/base.lua")

            simulation {
                world {
                    name = "gas_mix",
                    size = { 40, 40, 40 },
                    content = {
                        load_molecules { path = "Mods/Base/Molecules" },
                        random_fill {
                            density = 0.01,
                            region = box { size = fullworld - 4, center = center },
                            composition = { { name = molecule.h2o, fraction = 1.0 } }
                        }
                    }
                }
            }
        )");
        expect(ok, luaState.lastError());
        expect(simulation.worldTitle() == "gas_mix", "DSL world name should become world title");
        expect(simulation.atoms().size() > 0, "DSL gas world should spawn atoms");
    }

    void testLuaDslSimulationWorldLatticeBuildsScene() {
        Lattice::Simulation simulation;
        simulation.createWorld(glm::vec3(60.0f, 60.0f, 60.0f));

        Lattice::LuaState luaState;
        expect(luaState.valid(), "Lua state should be created");
        luaState.bindSimulation(simulation);

        const bool ok = luaState.runString(R"(
            dofile("Mods/Base/API/base.lua")
            local spacing = scene:lj_min(atom.C, atom.C)

            simulation {
                world {
                    name = "hex_lattice",
                    size = { 60, 60, 60 },
                    content = {
                        lattice_fill {
                            structure = "bcc",
                            region = box {
                                size = { spacing * 3 + 0.1, spacing * 3 + 0.1, spacing * 2 + 0.1 },
                                center = center,
                            },
                            margin = 0.0,
                            composition = { { name = atom.C, fraction = 1.0 } }
                        }
                    }
                }
            }
        )");
        expect(ok, luaState.lastError());
        expect(simulation.worldTitle() == "hex_lattice", "DSL lattice world name should become world title");
        expect(simulation.atoms().size() > 0, "DSL BCC lattice world should spawn atoms");
    }

    void testLuaDslSimulationWorldHexLatticeBuildsScene() {
        Lattice::Simulation simulation;
        simulation.createWorld(glm::vec3(60.0f, 60.0f, 60.0f));

        Lattice::LuaState luaState;
        expect(luaState.valid(), "Lua state should be created");
        luaState.bindSimulation(simulation);

        const bool ok = luaState.runString(R"(
            dofile("Mods/Base/API/base.lua")
            local spacing = scene:lj_min(atom.C, atom.C)

            simulation {
                world {
                    name = "hex_packing",
                    size = { 60, 60, 60 },
                    content = {
                        lattice_fill {
                            structure = "hex",
                            region = box {
                                size = { spacing * 3 + 0.1, spacing * 3 + 0.1, spacing * 2 + 0.1 },
                                center = center,
                            },
                            margin = 0.0,
                            composition = { { name = atom.C, fraction = 1.0 } }
                        }
                    }
                }
            }
        )");
        expect(ok, luaState.lastError());
        expect(simulation.worldTitle() == "hex_packing", "DSL hex lattice world name should become world title");
        expect(simulation.atoms().size() > 0, "DSL hex lattice world should spawn atoms");
    }
}

void runIoTests() {
    testLuaSceneObjectLoadsMoleculesDirectory();
    testLuaDslSimulationWorldGasBuildsScene();
    testLuaDslSimulationWorldLatticeBuildsScene();
    testLuaDslSimulationWorldHexLatticeBuildsScene();
}
